#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass, field
from typing import Optional, Dict, Any, List

@dataclass
class Location:
    file: str
    line: int = -1


@dataclass
class Symbol:
    id: str
    name: str
    kind: str
    module: str

    summary: str = ""
    detail: str = ""

    location: Optional[Location] = None

    tags: List[str] = field(default_factory=list)

    visibility: str = "public"
    deprecated: bool = False

    extra: Dict[str, Any] = field(default_factory=dict)