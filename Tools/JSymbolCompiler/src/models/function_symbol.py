#  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

from dataclasses import dataclass, field
from typing import List
from .base import Symbol

@dataclass
class Param:
    name: str
    type: str


@dataclass
class FunctionSymbol(Symbol):
    qualifiedName: str = ""
    returnType: str = ""

    params: List[Param] = field(default_factory=list)

    calls: List[str] = field(default_factory=list)
    calledBy: List[str] = field(default_factory=list)