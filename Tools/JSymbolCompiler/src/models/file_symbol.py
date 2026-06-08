#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass, field
from .base import Symbol

@dataclass
class FileSymbol(Symbol):
    includes: list[str] = field(default_factory=list)
    includedBy: list[str] = field(default_factory=list)
    symbols: list[str] = field(default_factory=list)