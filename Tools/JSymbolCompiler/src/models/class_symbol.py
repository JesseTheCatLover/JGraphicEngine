#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass, field
from typing import List
from .base import Symbol

@dataclass
class ClassSymbol(Symbol):
    bases: List[str] = field(default_factory=list)
    fields: List[str] = field(default_factory=list)
    methods: List[str] = field(default_factory=list)