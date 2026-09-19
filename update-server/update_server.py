#!/usr/bin/env python3
"""
NCM Converter 更新服务器 — 静态文件服务
监听 0.0.0.0:12345，提供 update.json 和更新包文件下载

用法:
    python update_server.py

配合 Cloudflare Tunnel 使用:
    cloudflared.exe tunnel --url http://localhost:12345
"""

import http.server
import os
import sys

PORT = 12345
DIRECTORY = os.path.dirname(os.path.abspath(__file__))


class UpdateHandler(http.server.SimpleHTTPRequestHandler):
    """静态文件处理器，增加 CORS 和 JSON Content-Type 支持"""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        # 允许跨域请求（NCM Converter 的 QNetworkAccessManager 可能需要）
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, HEAD, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def guess_type(self, path):
        # 确保 .json 返回 application/json
        if path.endswith(".json"):
            return "application/json; charset=utf-8"
        # .exe 返回正确的 MIME 类型
        if path.endswith(".exe"):
            return "application/octet-stream"
        return super().guess_type(path)

    def log_message(self, format, *args):
        # 带时间戳的日志
        import datetime
        timestamp = datetime.datetime.now().strftime("%H:%M:%S")
        sys.stderr.write(f"[{timestamp}] {format % args}\n")


if __name__ == "__main__":
    # 支持自定义端口: python update_server.py 8080
    port = int(sys.argv[1]) if len(sys.argv) > 1 else PORT

    server = http.server.HTTPServer(("0.0.0.0", port), UpdateHandler)
    print(f"")
    print(f"  NCM Converter Update Server")
    print(f"  ----------------------------")
    print(f"  Local:   http://localhost:{port}")
    print(f"  Network: http://0.0.0.0:{port}")
    print(f"  Files:   {DIRECTORY}")
    print(f"")
    print(f"  Press Ctrl+C to stop")
    print(f"")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n  Server stopped.")
        server.server_close()
