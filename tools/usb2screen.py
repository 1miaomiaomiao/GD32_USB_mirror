#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import ctypes.util
import glob
import os
import socket
import struct
import sys
import threading
import time
from collections import deque

import usb.backend.libusb1
import usb.core
import usb.util


def _load_backend():
    candidates = []
    try:
        import libusb
        root = os.path.dirname(libusb.__file__)
        candidates += glob.glob(os.path.join(root, "_platform", "_windows",
                                             "x64", "libusb-1.0.dll"))
        candidates += glob.glob(os.path.join(root, "x64", "libusb-1.0.dll"))
        candidates += glob.glob(os.path.join(root, "**", "libusb-1.0.dll"),
                                recursive=True)
    except ImportError:
        pass
    found = ctypes.util.find_library("libusb-1.0")
    if found:
        candidates.append(found)
    for dll in candidates:
        try:
            be = usb.backend.libusb1.get_backend(
                find_library=lambda _name, _p=dll: _p)
            if be is not None:
                print("Using libusb backend: %s" % dll)
                return be
        except Exception:
            continue
    sys.exit("No libusb-1.0 backend found. Run: pip install pyusb libusb")


USB_BACKEND = _load_backend()

VID = 0x28E9
PID = 0x028F

W, H = 480, 272
FRAME_BYTES = W * H * 2

CAM_W, CAM_H = 320, 240
CAM_FRAME_BYTES = CAM_W * CAM_H * 2
CAM_MAGIC = b"CAM"

EXIT_BTN = (418, 6, 474, 46)


def open_device(wait=True):
    dev = None
    while True:
        dev = usb.core.find(backend=USB_BACKEND, idVendor=VID, idProduct=PID)
        if dev is not None:
            break
        if not wait:
            sys.exit("Device %04x:%04x not found -- plug the board, check "
                     "Zadig (WinUSB driver on interface 0)" % (VID, PID))
        time.sleep(1.0)
    try:
        if dev.is_kernel_driver_active(0):
            try:
                dev.detach_kernel_driver(0)
            except Exception:
                pass
    except (NotImplementedError, usb.core.USBError):
        pass
    try:
        dev.set_configuration()
    except usb.core.USBError as e:
        if wait:
            time.sleep(1.0)
            return open_device(wait=True)
        raise
    cfg = dev.get_active_configuration()
    intf = cfg[(0, 0)]
    ep_out = usb.util.find_descriptor(intf, custom_match=lambda e:
                                      usb.util.endpoint_direction(e.bEndpointAddress)
                                      == usb.util.ENDPOINT_OUT
                                      and usb.util.endpoint_type(e.bmAttributes)
                                      == usb.util.ENDPOINT_TYPE_BULK)
    if ep_out is None:
        sys.exit("Bulk OUT endpoint not found -- descriptor mismatch")
    ep_in = usb.util.find_descriptor(intf, custom_match=lambda e:
                                     usb.util.endpoint_direction(e.bEndpointAddress)
                                     == usb.util.ENDPOINT_IN
                                     and usb.util.endpoint_type(e.bmAttributes)
                                     == usb.util.ENDPOINT_TYPE_BULK)
    print("Opened %04x:%04x, EP_OUT=0x%02x EP_IN=0x%02x, speed=%s"
          % (VID, PID, ep_out.bEndpointAddress,
             ep_in.bEndpointAddress if ep_in is not None else 0, dev.speed))
    return dev, ep_out, ep_in


def run_with_reconnect(stream_fn):
    dev = None
    while True:
        dev, ep_out, ep_in = open_device(wait=True)
        try:
            stream_fn(ep_out, ep_in)
            return
        except KeyboardInterrupt:
            print("\nStopped.")
            return
        except usb.core.USBError as e:
            print("USB link lost (%s) -- reconnecting in 1 s ..." % e)
            try:
                usb.util.dispose_resources(dev)
            except Exception:
                pass
            time.sleep(1.0)


def _is_usb_timeout(e):
    if getattr(e, "errno", None) is not None:
        try:
            if int(e.errno) in (-7, 60, 116, 10060):
                return True
        except (TypeError, ValueError):
            pass
    s = str(e).lower()
    return ("timeout" in s) or ("timed out" in s)


class CamViewer(object):

    def __init__(self):
        self.root = None
        self.label = None
        self._photo = None
        self._tk = None
        self._Image = None
        self._ImageTk = None
        self._np = None
        self._lut = None
        self._lock = threading.Lock()
        self._latest = None
        self._latest_seq = -1
        self.hidden = False
        self.disp_w = CAM_W * 2
        self.disp_h = CAM_H * 2

    def post_frame(self, data, seq):
        with self._lock:
            self._latest = data
            self._latest_seq = seq

    def _ensure(self):
        if self.root is not None:
            return
        import tkinter as tk
        from PIL import Image, ImageTk
        import numpy as np
        self._tk = tk
        self._Image = Image
        self._ImageTk = ImageTk
        self._np = np
        if self._lut is None:
            idx = np.arange(65536, dtype=np.uint16)
            r = ((idx >> 11) & 0x1F).astype(np.uint8)
            g = ((idx >> 5) & 0x3F).astype(np.uint8)
            b = (idx & 0x1F).astype(np.uint8)
            self._lut = np.stack(((r << 3) & 0xF8,
                                  (g << 2) & 0xFC,
                                  (b << 3) & 0xF8), axis=-1)
        self._mid_w = CAM_W * 2
        self._mid_h = CAM_H * 2
        root = tk.Tk()
        root.title("GD32 OV2640 USB Camera  (320x240 RGB565) -- Esc to hide")
        root.configure(bg="black")
        sw = root.winfo_screenwidth()
        sh = root.winfo_screenheight()
        fit = min(sw / float(CAM_W), sh / float(CAM_H))
        self.disp_w = int(CAM_W * fit)
        self.disp_h = int(CAM_H * fit)
        root.attributes("-fullscreen", True)
        label = tk.Label(root, bg="black", borderwidth=0)
        label.pack(expand=True)
        root.protocol("WM_DELETE_WINDOW", self._on_close)
        root.bind("<Escape>", lambda _e: self._on_close())
        self.root = root
        self.label = label
        self.hidden = False

    def _on_close(self):
        self.root.withdraw()
        self.hidden = True

    def _convert(self, data):
        np = self._np
        arr = np.frombuffer(data, dtype=np.uint16)
        rgb = self._lut[arr].reshape(CAM_H, CAM_W, 3)
        means = rgb.reshape(-1, 3).mean(0).astype(np.float32)
        gm = means.mean()
        scale = np.where(means > 0, gm / means, 0.0)
        scaled = np.clip(rgb.astype(np.float32) * scale, 0, 255)
        rgb = scaled.astype(np.uint8)
        img = self._Image.fromarray(rgb, "RGB")
        img = img.resize((self._mid_w, self._mid_h), self._Image.LANCZOS)
        return img.resize((self.disp_w, self.disp_h), self._Image.BILINEAR)

    def pump(self, state, stop_event, link_drop):
        self._ensure()
        self.hidden = False
        self.root.deiconify()
        self.root.attributes("-fullscreen", True)
        last_seq = -1
        while (state["mode"] == 3 and not stop_event.is_set()
               and not link_drop.is_set()):
            with self._lock:
                data, seq = self._latest, self._latest_seq
            if data is not None and seq != last_seq and not self.hidden:
                last_seq = seq
                try:
                    self._photo = self._ImageTk.PhotoImage(self._convert(data))
                    self.label.configure(image=self._photo)
                except Exception as e:
                    print("viewer render error: %s" % e)
            try:
                self.root.update_idletasks()
                self.root.update()
            except Exception:
                pass
            time.sleep(0.01)
        if not self.hidden:
            self.root.withdraw()


def stream_loop(ep_out, ep_in, make_frame, fps=30, label=""):
    period = 1.0 / fps
    t_tx = 0.0
    t = 0
    try:
        while True:
            frame = make_frame(t)
            ep_out.write(frame, timeout=2000)
            t += 1
            t_tx += period
            delay = t_tx - time.time()
            if delay > 0:
                time.sleep(delay)
    except KeyboardInterrupt:
        print("\nStopped.")


PANEL_BG = (16, 22, 29)
PANEL_BORDER = (30, 42, 54)
BG = (10, 14, 19)
TXT = (229, 237, 245)
DIM = (140, 160, 179)
CYAN = (34, 211, 238)
ORANGE = (251, 146, 60)
GREEN = (52, 211, 153)
AMBER = (251, 191, 36)
RED = (248, 113, 113)

_COL = (8, 44, 146, 268)
_CMID = (154, 44, 330, 268)
_CRIT = (338, 44, 472, 268)


def _load_fonts():
    from PIL import ImageFont
    fonts = {}
    candidates = {
        "m": ["consola.ttf"],
        "mb": ["consolab.ttf"],
        "s": ["segoeui.ttf", "arial.ttf"],
        "sb": ["seguisb.ttf", "segoeuib.ttf", "arialbd.ttf"],
    }
    for key, names in candidates.items():
        for name in names:
            path = os.path.join(os.environ.get("WINDIR", r"C:\Windows"),
                                "Fonts", name)
            if os.path.exists(path):
                fonts[key] = path
                break
    def get(key, size):
        path = fonts.get(key)
        if path:
            return ImageFont.truetype(path, size)
        return ImageFont.load_default()
    return get


def _load_color(v):
    v = max(0.0, min(100.0, v)) / 100.0
    if v < 0.5:
        t = v / 0.5
        return tuple(int(GREEN[i] + (AMBER[i] - GREEN[i]) * t) for i in range(3))
    t = (v - 0.5) / 0.5
    return tuple(int(AMBER[i] + (RED[i] - AMBER[i]) * t) for i in range(3))


def _ecg_template(n=64):
    import math
    out = []
    for i in range(n):
        x = i / (n - 1)
        v = 0.0
        v += 0.12 * math.exp(-((x - 0.16) ** 2) / 0.0008)
        v += -0.14 * math.exp(-((x - 0.30) ** 2) / 0.0003)
        v += 1.00 * math.exp(-((x - 0.34) ** 2) / 0.00012)
        v += -0.22 * math.exp(-((x - 0.39) ** 2) / 0.0004)
        v += 0.22 * math.exp(-((x - 0.60) ** 2) / 0.0030)
        out.append(v)
    return out


class PcMonitor:

    HIST = 150
    CHART_RECT = (160, 62, 324, 146)
    NET_RECT = (160, 176, 324, 238)

    def __init__(self):
        import psutil
        self.ps = psutil
        self.hostname = socket.gethostname()[:18]
        self.cores = psutil.cpu_count(logical=True) or 1
        self.cpu_hist = deque([0.0] * self.HIST, maxlen=self.HIST)
        self.down_hist = deque([0.0] * self.HIST, maxlen=self.HIST)
        self.up_hist = deque([0.0] * self.HIST, maxlen=self.HIST)

        self.cpu_percpu = [0.0] * self.cores
        self.cpu_total = 0.0
        self.mem_pct = 0.0
        self.down = 0.0
        self.up = 0.0
        self.disk_r = 0.0
        self.disk_w = 0.0
        self.disk_pct = 0.0
        self.nproc = 0
        self.bat = None
        self.lock = threading.Lock()

        try:
            self.disk_pct = psutil.disk_usage("C:\\").percent
        except Exception:
            self.disk_pct = 0.0
        try:
            b = psutil.sensors_battery()
            if b is not None:
                self.bat = (b.percent, bool(b.power_plugged))
        except Exception:
            self.bat = None

        self._last_net = psutil.net_io_counters()
        self._last_dsk = psutil.disk_io_counters()
        self._last_t = time.time()
        self._tick = 0
        self._stop = threading.Event()
        self.sampler = threading.Thread(target=self._sample_loop, daemon=True)
        self.sampler.start()

        self._fonts = _load_fonts()
        self._ecg = _ecg_template()
        self._ecg_pos = 0.0
        self._ecg_hist = deque([0.0] * 150, maxlen=150)
        self._bg = self._build_bg()

    def _sample_loop(self):
        ps = self.ps
        ps.cpu_percent(interval=None, percpu=True)
        while not self._stop.is_set():
            t = time.time()
            dt = max(t - self._last_t, 1e-3)
            percpu = ps.cpu_percent(interval=None, percpu=True)
            mem = ps.virtual_memory().percent
            net = ps.net_io_counters()
            dsk = ps.disk_io_counters()
            down = (net.bytes_recv - self._last_net.bytes_recv) / dt / 1e6
            up = (net.bytes_sent - self._last_net.bytes_sent) / dt / 1e6
            dr = (dsk.read_bytes - self._last_dsk.read_bytes) / dt / 1e6
            dw = (dsk.write_bytes - self._last_dsk.write_bytes) / dt / 1e6
            self._last_net, self._last_dsk, self._last_t = net, dsk, t
            nproc = len(ps.pids()) if self._tick % 10 == 0 else self.nproc

            with self.lock:
                self.cpu_percpu = percpu
                self.cpu_total = sum(percpu) / max(len(percpu), 1)
                self.mem_pct = mem
                self.down, self.up = max(down, 0.0), max(up, 0.0)
                self.disk_r, self.disk_w = max(dr, 0.0), max(dw, 0.0)
                self.nproc = nproc
                self.cpu_hist.append(self.cpu_total)
                self.down_hist.append(self.down)
                self.up_hist.append(self.up)
            self._tick += 1
            self._stop.wait(0.2)

    def _build_bg(self):
        from PIL import Image, ImageDraw
        img = Image.new("RGB", (W, H), BG)
        d = ImageDraw.Draw(img)
        get = self._fonts

        for (x0, y0, x1, y1) in (_COL, _CMID, _CRIT):
            d.rounded_rectangle((x0, y0, x1, y1), radius=8, fill=PANEL_BG,
                                outline=PANEL_BORDER, width=1)

        d.rounded_rectangle((0, 0, W - 1, 36), radius=8, fill=(13, 18, 24),
                            outline=PANEL_BORDER, width=1)
        d.text((26, 11), "SYSTEM PULSE", font=get("sb", 13), fill=CYAN)
        d.text((_COL[0] + 69, 246), self.hostname, font=get("s", 11),
               fill=DIM, anchor="mm")

        d.text((160, 50), "CPU LOAD · 30 s", font=get("s", 11), fill=DIM)
        d.text((160, 162), "NETWORK · MB/s", font=get("s", 11), fill=DIM)
        d.text((_CRIT[0] + 10, 52), "NET NOW", font=get("s", 11), fill=DIM)
        d.text((_CRIT[0] + 10, 118), "DISK  (C: %d%%)" % self.disk_pct,
               font=get("s", 11), fill=DIM)
        d.text((_CRIT[0] + 10, 186), "SYSTEM", font=get("s", 11), fill=DIM)

        d.text((_COL[0] + 69, 58), "CPU", font=get("s", 11), fill=DIM,
               anchor="mm")
        d.arc((31, 74, 123, 166), start=150, end=390, fill=(34, 46, 58),
              width=9)

        d.text((_COL[0] + 12, 214), "MEM", font=get("s", 11), fill=DIM)
        d.rounded_rectangle((_COL[0] + 12, 228, _COL[0] + 126, 240),
                            radius=5, fill=(24, 33, 42))

        d.text((160, 246), "DOWN", font=get("s", 10), fill=CYAN)
        d.text((210, 246), "UP", font=get("s", 10), fill=ORANGE)

        x0, y0, x1, y1 = EXIT_BTN
        d.rounded_rectangle(EXIT_BTN, radius=8, fill=(23, 32, 41),
                            outline=(62, 78, 94), width=1)
        d.line((x0 + 15, y0 + 12, x1 - 15, y1 - 12), fill=TXT, width=3)
        d.line((x1 - 15, y0 + 12, x0 + 15, y1 - 12), fill=TXT, width=3)
        return img

    def render(self, t):
        from PIL import Image, ImageDraw
        get = self._fonts
        with self.lock:
            cpu = self.cpu_total
            percpu = list(self.cpu_percpu)
            mem = self.mem_pct
            down, up = self.down, self.up
            dr, dw = self.disk_r, self.disk_w
            nproc = self.nproc
            cpu_hist = list(self.cpu_hist)
            down_hist = list(self.down_hist)
            up_hist = list(self.up_hist)

        img = self._bg.copy()
        d = ImageDraw.Draw(img)
        col = _load_color(cpu)

        d.ellipse((8, 10, 18, 20),
                  fill=(34, 197, 94) if (t // 10) % 2 == 0 else (20, 90, 50))
        bpm = 42 + cpu * 1.1
        self._ecg_pos += bpm / 60.0 * len(self._ecg) / 30.0
        while self._ecg_pos >= len(self._ecg):
            self._ecg_pos -= len(self._ecg)
        self._ecg_hist.append(self._ecg[int(self._ecg_pos)])
        ex0, ex1, ey0, ey1 = 118, 262, 8, 30
        pts = []
        n = len(self._ecg_hist)
        for i, v in enumerate(self._ecg_hist):
            x = ex0 + (ex1 - ex0) * i / (n - 1)
            y = ey1 - 3 - v * (ey1 - ey0 - 6)
            pts.append((x, y))
        d.line(pts, fill=col, width=1, joint="curve")
        d.text((ex1 + 6, 18), "%d" % round(bpm), font=get("mb", 12),
               fill=col, anchor="lm")
        d.text((ex1 + 6, 28), "BPM", font=get("s", 8), fill=DIM, anchor="lm")

        lt = time.localtime()
        sep = ":" if (t // 15) % 2 == 0 else " "
        d.text((EXIT_BTN[0] - 12, 19),
               "%02d%s%02d%s%02d" % (lt.tm_hour, sep, lt.tm_min, sep,
                                     lt.tm_sec),
               font=get("mb", 19), fill=TXT, anchor="rm")

        cx, cy = 77, 120
        d.arc((31, 74, 123, 166), start=150, end=150 + 240 * cpu / 100.0,
              fill=col, width=9)
        d.text((cx, cy - 6), "%d" % round(cpu), font=get("mb", 30),
               fill=col, anchor="mm")
        d.text((cx, cy + 16), "PERCENT", font=get("s", 8), fill=DIM,
               anchor="mm")

        n = min(len(percpu), 16)
        bw = 7 if n > 10 else 13
        gap = 2
        total_w = n * bw + (n - 1) * gap
        x = (_COL[0] + _COL[2]) // 2 - total_w // 2
        for i in range(n):
            v = percpu[i] if i < len(percpu) else 0
            bh = int(30 * v / 100.0)
            c = _load_color(v)
            d.rectangle((x, 208 - bh, x + bw - 1, 208), fill=c)
            x += bw + gap

        mw = int(114 * mem / 100.0)
        if mw > 0:
            d.rounded_rectangle((_COL[0] + 12, 228, _COL[0] + 12 + mw, 240),
                                radius=5, fill=_load_color(mem))
        d.text((_COL[2] - 12, 222), "%d%%" % round(mem), font=get("mb", 11),
               fill=TXT, anchor="rm")

        x0, y0, x1, y1 = self.CHART_RECT
        m = len(cpu_hist)
        pts = [(x0 + (x1 - x0) * i / (m - 1),
                y1 - (y1 - y0 - 4) * min(cpu_hist[i], 100.0) / 100.0)
               for i in range(m)]
        d.polygon(pts + [(x1, y1), (x0, y1)], fill=(23, 42, 52))
        d.line(pts, fill=CYAN, width=2, joint="curve")

        x0, y0, x1, y1 = self.NET_RECT
        peak = max(max(down_hist), max(up_hist), 0.5)
        scale = (y1 - y0 - 4) / peak
        m = len(down_hist)
        for hist, c in ((down_hist, CYAN), (up_hist, ORANGE)):
            pts = [(x0 + (x1 - x0) * i / (m - 1),
                    y1 - min(hist[i], peak) * scale) for i in range(m)]
            d.line(pts, fill=c, width=2, joint="curve")

        rx = _CRIT[0] + 10
        d.text((rx, 70), "DOWN", font=get("s", 10), fill=CYAN)
        d.text((rx + 44, 70), "%.1f MB/s" % down, font=get("mb", 13),
               fill=CYAN)
        d.text((rx, 94), "UP", font=get("s", 10), fill=ORANGE)
        d.text((rx + 44, 94), "%.1f MB/s" % up, font=get("mb", 13),
               fill=ORANGE)

        d.text((rx, 136), "R %.1f" % dr, font=get("mb", 13), fill=TXT)
        d.text((rx, 160), "W %.1f MB/s" % dw, font=get("mb", 13), fill=TXT)

        up_s = time.time() - self.ps.boot_time()
        days, rem = divmod(int(up_s), 86400)
        hh, rem = divmod(rem, 3600)
        mm, ss = divmod(rem, 60)
        d.text((rx, 204), "UP %dd %02d:%02d:%02d" % (days, hh, mm, ss),
               font=get("mb", 12), fill=TXT)
        line2 = "PROC %d" % nproc
        if self.bat is not None:
            line2 += "   BAT %d%%%s" % (self.bat[0],
                                        "+" if self.bat[1] else "-")
        d.text((rx, 226), line2, font=get("mb", 12), fill=TXT)

        return pack565(img)


_PACK_BUFS = {}
_BAYER_TILE = {}


def _bayer_tile(h, w):
    import numpy as np
    t = _BAYER_TILE.get((h, w))
    if t is None:
        b = np.array([[0, 8, 2, 10],
                      [12, 4, 14, 6],
                      [3, 11, 1, 9],
                      [15, 7, 13, 5]], dtype=np.float32)
        b = (b + 0.5) / 16.0 - 0.5
        t = np.tile(b, ((h + 3) // 4, (w + 3) // 4))[:h, :w]
        _BAYER_TILE[(h, w)] = t
    return t


def _rgb_to_565(img, dither=False, key="main"):
    import numpy as np
    arr = np.asarray(img, dtype=np.uint16)
    buf = _PACK_BUFS.get(key)
    if buf is None or buf.shape != arr.shape[:2]:
        buf = np.empty(arr.shape[:2], dtype="<u2")
        _PACK_BUFS[key] = buf
    if dither:
        f = arr.astype(np.float32)
        d = _bayer_tile(*arr.shape[:2])
        for ch, lsb in ((0, 8.0), (1, 4.0), (2, 8.0)):
            v = f[..., ch]
            att = 1.0 - np.minimum(v, 255.0 - v) / 32.0
            np.clip(att, 0.0, 1.0, out=att)
            f[..., ch] += d * lsb * att
        np.clip(f, 0.0, 255.0, out=f)
        arr = f.astype(np.uint16)
    np.copyto(buf, arr[..., 2], casting="unsafe")
    np.right_shift(buf, 3, out=buf)
    buf |= (arr[..., 1] & np.uint16(0xFC)) << 3
    buf |= (arr[..., 0] & np.uint16(0xF8)) << 8
    return buf


def pack565(img):
    return _rgb_to_565(img).tobytes()


def stream_monitor(ep_out, ep_in, fps=30):
    try:
        import psutil
        import numpy
        from PIL import Image
    except ImportError as e:
        sys.exit("Monitor mode needs psutil + numpy + pillow "
                 "(%s): pip install psutil numpy pillow" % e)
    mon = PcMonitor()
    stream_loop(ep_out, ep_in, mon.render, fps=fps, label="monitor dashboard")


def _rgb565_le(r, g, b):
    px = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
    return bytes((px & 0xFF, px >> 8))


_BAR_COLORS = [(0xFF, 0x00, 0x00), (0x00, 0xFF, 0x00), (0x00, 0x00, 0xFF),
               (0xFF, 0xFF, 0x00), (0xFF, 0x00, 0xFF), (0x00, 0xFF, 0xC8),
               (0xFF, 0xFF, 0xFF), (0x40, 0x40, 0x40)]
_BLACK2 = _rgb565_le(0, 0x00, 0x00)
_WHITE2 = _rgb565_le(0xFF, 0xFF, 0xFF)
_ORANGE2 = _rgb565_le(0xFF, 0xA5, 0x00)


def _build_base_frame():
    bar_w = W // len(_BAR_COLORS)
    row = bytearray(W * 2)
    for i, (r, g, b) in enumerate(_BAR_COLORS):
        row[i * bar_w * 2:(i + 1) * bar_w * 2] = _rgb565_le(r, g, b) * bar_w
    row[0:8 * 2] = _BLACK2 * 8
    row[(W - 8) * 2:] = _BLACK2 * 8
    bar_row = bytes(row)
    black_row = _BLACK2 * W
    base = bytearray(FRAME_BYTES)
    for y in range(H):
        base[y * W * 2:(y + 1) * W * 2] = (
            black_row if (y < 8 or y >= H - 8) else bar_row)
    return bytes(base)


BASE_FRAME = _build_base_frame()


def make_test_frame(t):
    frame = bytearray(BASE_FRAME)

    mv = (t * 8) % W
    x0 = max(mv, 8)
    x1 = min(mv + 24, W - 8)
    if x1 > x0:
        seg = _WHITE2 * (x1 - x0)
        for y in range(8, H - 8):
            off = y * W * 2
            frame[off + x0 * 2:off + x1 * 2] = seg

    if (t // 15) % 2 == 0:
        seg = _ORANGE2 * 40
        for y in range(0, 40):
            off = y * W * 2
            frame[off:off + 40 * 2] = seg

    return bytes(frame)


def stream_pattern(ep_out, fps=30):
    stream_loop(ep_out, make_test_frame, fps=fps, label="test pattern")


def _overlay_exit_badge(a):
    import numpy as np
    dark = np.uint16(0x4A69)
    edge = np.uint16(0x7BEF)
    white = np.uint16(0xFFFF)
    x0, y0, x1, y1 = EXIT_BTN
    a[y0:y1 + 1, x0:x1 + 1] = dark
    a[y0, x0:x1 + 1] = edge
    a[y1, x0:x1 + 1] = edge
    a[y0:y1 + 1, x0] = edge
    a[y0:y1 + 1, x1] = edge
    for t in range(0, 17):
        yy = y0 + 12 + t
        xa = x0 + 13 + t
        xb = x1 - 13 - t
        a[yy, xa - 1:xa + 2] = white
        a[yy, xb - 1:xb + 2] = white
    return a


def frame_from_bgra(rgb_bytes, size):
    from PIL import Image, ImageFilter
    img = Image.frombytes("RGB", size, rgb_bytes)
    f = max(1, min(size[0] // W, size[1] // H))
    if f >= 2:
        img = img.reduce(f)
    img = img.resize((W, H), Image.LANCZOS)
    img = img.filter(ImageFilter.UnsharpMask(radius=1.4, percent=80,
                                             threshold=3))
    return _overlay_exit_badge(
        _rgb_to_565(img, dither=True, key="mirror")).tobytes()


class MirrorSource(object):

    def __init__(self, monitor_idx=1):
        import mss
        self.sct = mss.mss()
        if monitor_idx >= len(self.sct.monitors):
            raise RuntimeError("monitor %d not found -- only %d monitor(s)"
                               % (monitor_idx, len(self.sct.monitors) - 1))
        self.mon = self.sct.monitors[monitor_idx]
        self.latest = None
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self._thread = None
        self.frames = 0

    def _capture_loop(self):
        probed = False
        while not self._stop.is_set():
            try:
                shot = self.sct.grab(self.mon)
                if not probed:
                    probed = True
                    try:
                        from PIL import Image as _I
                        _I.frombytes("RGB", shot.size, shot.rgb).save(
                            os.path.join(os.path.dirname(
                                os.path.abspath(__file__)),
                                "mirror_probe.png"))
                    except Exception:
                        pass
                frame = frame_from_bgra(shot.rgb, shot.size)
                with self._lock:
                    self.latest = frame
                self.frames += 1
            except Exception as e:
                print("capture error: %s" % e)
                time.sleep(0.1)

    def start(self):
        if self._thread is None:
            self._stop.clear()
            self._thread = threading.Thread(target=self._capture_loop,
                                            daemon=True)
            self._thread.start()

    def stop(self):
        self._stop.set()
        if self._thread is not None:
            self._thread.join(timeout=1.0)
            self._thread = None
        with self._lock:
            self.latest = None


def stream_mirror(ep_out, ep_in, monitor=1):
    try:
        import mss
    except ImportError:
        sys.exit("Mirror mode needs mss (and numpy + Pillow): "
                 "pip install mss numpy pillow")

    src = MirrorSource(monitor)
    src.start()

    last = None
    try:
        while True:
            with src._lock:
                frame = src.latest
            if frame is not None and frame is not last:
                ep_out.write(frame, timeout=2000)
                last = frame
            else:
                time.sleep(0.002)
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        src.stop()


MODE_NAMES = ["STOP", "MONITOR", "MIRROR", "CAMERA"]


def stream_host_controlled(ep_out, ep_in, mon, viewer, fps=30):
    if ep_in is None:
        sys.exit("Board control needs a bulk IN endpoint -- descriptor mismatch")
    state = {"mode": 0}
    stop_write = threading.Event()
    ctl_stop = threading.Event()
    link_drop = threading.Event()

    def apply_mode(m):
        if m in (0, 1, 2, 3) and m != state["mode"]:
            state["mode"] = m
        if m == 0 or m == 3:
            stop_write.set()
        else:
            stop_write.clear()

    def ctl_loop():
        while not ctl_stop.is_set() and not link_drop.is_set():
            try:
                data = ep_in.read(64, timeout=200)
                if data:
                    m = int(data[0])
                    if m in (0, 1, 2, 3):
                        apply_mode(m)
            except usb.core.USBError as e:
                if _is_usb_timeout(e):
                    pass
                else:
                    print("bulk-IN read error: %s -- reconnecting" % e)
                    link_drop.set()
                    return

            while (state["mode"] == 3 and not ctl_stop.is_set()
                   and not link_drop.is_set()):
                try:
                    hdr = ep_in.read(8, timeout=1000)
                except usb.core.USBError as e:
                    if _is_usb_timeout(e):
                        continue
                    print("camera header read error: %s -- reconnecting" % e)
                    link_drop.set()
                    return
                if not hdr:
                    continue
                if len(hdr) < 8 or bytes(hdr[0:3]) != CAM_MAGIC:
                    if len(hdr) <= 4 and int(hdr[0]) in (0, 1, 2, 3):
                        apply_mode(int(hdr[0]))
                    continue
                (plen,) = struct.unpack("<I", bytes(hdr[4:8]))
                if plen != CAM_FRAME_BYTES:
                    print("camera frame length mismatch: %d != %d" %
                          (plen, CAM_FRAME_BYTES))
                    continue
                try:
                    payload = ep_in.read(plen, timeout=3000)
                except usb.core.USBError as e:
                    if _is_usb_timeout(e):
                        continue
                    print("camera payload read error: %s -- reconnecting" % e)
                    link_drop.set()
                    return
                if len(payload) != plen:
                    continue
                viewer.post_frame(bytes(payload), int(hdr[3]))

    ctl = threading.Thread(target=ctl_loop, daemon=True)
    ctl.start()

    mirror = None
    period = 1.0 / fps
    t_tx = 0.0
    t = 0
    try:
        while not link_drop.is_set():
            m = state["mode"]

            if m == 3:
                viewer.pump(state, ctl_stop, link_drop)
                if link_drop.is_set():
                    raise usb.core.USBError("link lost during camera stream")
                continue

            if stop_write.is_set():
                if mirror is not None:
                    mirror.stop()
                    mirror = None
                t_tx = time.time()
                time.sleep(0.01)
                continue

            if m != 2 and mirror is not None:
                mirror.stop()
                mirror = None

            frame = None
            if m == 2:
                if mirror is None:
                    mirror = MirrorSource(1)
                    mirror.start()
                with mirror._lock:
                    frame = mirror.latest
            elif m == 1:
                frame = mon.render(t)

            if frame is not None:
                ep_out.write(frame, timeout=2000)
                t += 1
            else:
                time.sleep(0.002)

            t_tx += period
            delay = t_tx - time.time()
            if delay > 0:
                time.sleep(delay)

        if link_drop.is_set():
            raise usb.core.USBError("bulk-IN link lost (ctl thread)")
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ctl_stop.set()
        if mirror is not None:
            mirror.stop()


def make_frame_from_image(path):
    from PIL import Image
    img = Image.open(path).convert("RGB").resize((W, H))
    return pack565(img)


def stream_image(ep_out, ep_in, path, fps=15):
    frame = make_frame_from_image(path)
    stream_loop(ep_out, ep_in, lambda t: frame, fps=fps, label="still image")


def main():
    if sys.platform == "win32":
        try:
            import ctypes
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass

    import gc
    gc.collect()
    gc.freeze()

    ap = argparse.ArgumentParser(description="GD32 USB secondary display")
    ap.add_argument("--mode", default="host",
                    choices=["host", "monitor", "pattern", "mirror", "image"],
                    help="host = board-controlled via panel menu (default); "
                         "monitor/pattern/mirror/image = fixed-source debug")
    ap.add_argument("--image", help="image file for --mode image")
    ap.add_argument("--monitor", type=int, default=1,
                    help="monitor index to mirror (1 = primary, 0 = all)")
    ap.add_argument("--fps", type=int, default=30, help="target frame rate")
    args = ap.parse_args()

    try:
        if args.mode == "host":
            try:
                import psutil
                import numpy
                from PIL import Image
            except ImportError as e:
                sys.exit("Needs psutil + numpy + pillow + mss (%s): "
                         "pip install psutil numpy pillow mss" % e)
            mon = PcMonitor()
            viewer = CamViewer()
            run_with_reconnect(lambda o, i: stream_host_controlled(
                o, i, mon, viewer, fps=args.fps))
        elif args.mode == "mirror":
            run_with_reconnect(lambda o, i: stream_mirror(o, i, args.monitor))
        elif args.mode == "image" and args.image:
            frame = make_frame_from_image(args.image)
            run_with_reconnect(lambda o, i: stream_loop(
                o, i, lambda t: frame, fps=args.fps, label="still image"))
        elif args.mode == "pattern":
            run_with_reconnect(lambda o, i: stream_loop(
                o, i, make_test_frame, fps=args.fps, label="test pattern"))
        else:
            run_with_reconnect(lambda o, i: stream_monitor(o, i, fps=args.fps))
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
