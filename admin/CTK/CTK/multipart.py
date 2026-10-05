"""Small replacement for the removed :mod:`cgi` form parser."""

from __future__ import annotations

from email import policy
from email.parser import BytesParser
from io import BytesIO
from typing import Any, BinaryIO, Optional, cast
from urllib.parse import parse_qs


class FieldStorage:
    def __init__(self, fp=None, headers=None, outerboundary="", environ=None,
                 keep_blank_values=0, strict_parsing=0):
        self._fields: dict[str, _Field] = {}
        environ = environ or {}
        body = fp.read(int(environ.get("CONTENT_LENGTH", 0))) if fp else b""
        if isinstance(body, str):
            body = body.encode()
        content_type = environ.get("CONTENT_TYPE", "")
        if content_type.startswith("multipart/"):
            message = BytesParser(policy=policy.default).parsebytes(
                f"Content-Type: {content_type}\r\nMIME-Version: 1.0\r\n\r\n".encode() + body)
            for part in message.iter_parts():
                disposition = part.get("Content-Disposition", "")
                name = part.get_param("name", header="content-disposition")
                if not isinstance(name, str):
                    continue
                item = _Field()
                item.filename = part.get_filename()
                item.value = cast(bytes, part.get_payload(decode=True) or b"")
                self.filename = item.filename
                item.file = self.make_file() if item.filename else BytesIO(item.value)
                if item.filename:
                    item.file.write(item.value)
                    item.file.seek(0)
                self._fields[name] = item
        else:
            for name, values in parse_qs(body.decode(), keep_blank_values=bool(keep_blank_values)).items():
                item = _Field()
                item.value = values[0]
                item.file = BytesIO(item.value.encode())
                self._fields[name] = item

    def make_file(self, binary=None):
        return BytesIO()

    def __getitem__(self, name):
        return self._fields[name]


class _Field:
    filename: Optional[str] = None
    value: Any = ''
    file: Optional[BinaryIO] = None
