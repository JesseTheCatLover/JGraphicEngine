#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass
from .base import Symbol

@dataclass
class FieldSymbol(Symbol):
    type: str = ""
    owner: str = ""