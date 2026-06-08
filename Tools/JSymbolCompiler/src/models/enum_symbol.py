#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass, field
from .base import Symbol

@dataclass
class EnumSymbol(Symbol):
    values: list[str] = field(default_factory=list)