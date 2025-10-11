//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JApplication.h"

int main(int argc, char** argv)
{
    //  Uses CLI args: --editor or --game; Dynamically choose mode (Editor/Game)
    return JApplication::RunFromArgs(argc, argv) ? 0 : 1;
}
