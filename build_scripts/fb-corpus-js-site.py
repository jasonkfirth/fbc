#!/usr/bin/env python3
"""Package verified programs for the FreeBASIC browser corpus.

Write a static catalog, a launch page per program, and corresponding source
archives for explicitly reviewed entries. Apply presentation updates to an
existing release without recompiling its programs. The build driver and the
site operator handle compilation and publishing.
"""

import argparse
import hashlib
import html
import json
import os
import re
import shutil
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile


# Program text describes the behavior exercised by the browser checks.
# The final tuple field selects the project's notice to ship with the source.
PROGRAMS = {
    "pyromax-dax": ("Pyromax Dax", "Play the original 2.5D platformer through its campaign maps.",
                    "Press Enter to start, then use the arrow keys to move, Space to jump, and Z to fire.", "LICENSE"),
    "darkcastle": ("Dark Castle", "Explore a playable dungeon room in the original isometric game engine.",
                   "Start the adventure from the game menu. This browser build includes a new room and textures.", "LICENSE"),
    "openmidisesh": ("OpenMIDISeSh", "Write, edit, and play MIDI scores in the original graphical editor.",
                    "Use the built-in synth. SoundFont streaming and hardware MIDI are unavailable in the browser build.", "LICENSE"),
    "fbrt-format": ("fbrtLib Format", "Try fbrtLib's numeric formatting with your own value and format mask.",
                    "Start with the example mask for grouped digits and two decimal places.", "license.txt"),
    "fgconvert": ("FlightGear Pilot Conversions", "Convert altitudes, distances and angles, or calculate simulator descent distance.",
                  "Choose an operation and enter your own values.", "LICENSE.md"),
    "fbfrog": ("fbfrog Binding Generator", "Convert C header declarations to FreeBASIC bindings.",
               "Paste or open a self-contained C header and download the generated .bi file. "
               "Review the declarations before using them in a project.", "license.txt"),
    "clumsier": ("Clumsier", "Edit a FreeBASIC source file in Clumsier's text editor.",
                 "Use a keyboard to edit the sample or import a .bas file. Save in this browser "
                 "and download a copy when you finish. Compiling requires a desktop FreeBASIC installation.",
                 "LICENSE"),
    "hackit": ("HACKIT", "Edit BASIC source in the original syntax-highlighting console editor.",
               "Use a physical keyboard. Open the sample or a local ASCII file, press Ctrl+S to save, "
               "then download the saved copy. Compiling requires a desktop FreeBASIC installation.",
               "LICENSE.TXT"),
    "griffon-legend": ("The Griffon Legend", "Explore a town and its dungeons in Syn9's action RPG.",
                      "Press Enter at the title, then choose New Game. Esc skips the story or opens the game menu. "
                      "Use the save and load menu to keep your progress in this browser. The browser build is silent.",
                      "COPYING"),
    "phoenix": ("Dungeon of the Phoenix", "Explore Syn9's roguelike dungeon, collect keys, and fight monsters.",
                "Use a keyboard. Press Space twice to start, move with the arrow keys, and bump into monsters to attack. "
                "The browser edition uses letter and symbol tiles with the original music and sound effects.", "LICENSE"),
    "basicverse-island": ("BASICVERSE Island", "Explore a textured island rendered in software.",
                         "Use W, A, S and D to move, and the mouse to look around. "
                         "The browser build uses a smaller canvas and has no NPC dialogue model loaded.",
                         "LICENSE"),
    "imageproc-grayscale": ("fb-imageproc Grayscale", "Convert a photo with four FreeBASIC grayscale methods.",
                            "Try the sample or open your own image, then download a grayscale PNG.",
                            "LICENSE"),
    "ext-list": ("FreeBASIC Extended Library List", "Build a list and remove its even values.",
                 "The output shows the list before and after filtering.", "COPYING"),
    "catfish": ("Catfish Bouncer", "Keep a bouncing ball in play with four paddles.",
                "Move the mouse over the game to position the paddles.", "LICENSE"),
    "chessbrain": ("ChessbrainFB", "Play chess against the ChessbrainFB engine.",
                   "Drag a white piece to a legal square. The engine plays black.", "LICENSE"),
    "nebunoid": ("Nebunoid", "Move a paddle and clear the bricks across the original campaigns.",
                 "Choose Play local game and a campaign, then press 1 for a single player. "
                 "Move the mouse to steer and press Space to serve. The browser build is silent "
                 "and uses NASA's Flame Nebula background.", "license.md"),
    "ohrrpgce-game": ("OHRRPGCE Game Player", "Run the engine's illustrated keyboard test RPG.",
                      "Click the game canvas, then follow the controls shown at the bottom of the screen.",
                      "LICENSE.txt"),
    "tilengine-learning": ("FBTilengine Layer Demo", "Watch Tilengine blend and move two bitmap layers.",
                           "The animation starts when the player loads.", "LICENSE"),
    "tilengine-mode7": ("Tilengine Mode 7", "Explore a tilemap drawn with a different perspective transform on each scanline.",
                        "Use Up and Down to move, Left and Right to turn, and Z to reset the view.", "LICENSE"),
    "tiko": ("Tiko Editor", "Open and edit a syntax-highlighted FreeBASIC source file.",
             "Use a keyboard. File > Save stores the sample in this browser. Use Download saved example.bas "
             "to keep a copy. Compiling and running programs require the desktop edition.", "LICENSE"),
    "ugomoku": ("Ugomoku", "Play five in a row against A. Millett's original BASIC computer opponent.",
                 "Press Space to start. Use the arrow keys or J, K, O and L to move, then Enter to place a piece.",
                 "LICENSE"),
    "strawberrynes": ("StrawberryNES", "Run an original test cartridge in the FreeBASIC NES emulator.",
                      "Hold the Right arrow to turn the NES picture green. Release it to return to blue.", "license.txt"),
    "fbsound-eq": ("FBSound Equalizer", "Listen to a test clip with and without FBSound's equalizer.",
                   "Press 1 for the original, 2 to boost 750 Hz, or 3 to cut it. The upper waveform is the original. "
                   "The lower waveform shows the selected output.", "license.txt"),
    "raylib-camera": ("raylib First-Person Camera", "Explore a 3D scene drawn with raylib.",
                      "Use W, A, S and D to move through the scene.", "LICENSE"),
    "raylib-window": ("raylib Basic Window", "Render the original raylib window and text example.",
                      "The canvas shows the output as soon as the program starts.", "LICENSE"),
    "rekenblad": ("Rekenblad", "Edit a spreadsheet with formulas and sample budgets.",
                  "Use the arrow keys to select a cell, F2 to edit and Enter to apply. "
                  "Ctrl+S saves the workbook in this browser.",
                  "LICENSE"),
    "schrijf": ("Schrijf", "Edit a sample document in a keyboard-driven word processor.",
                "Type into the document or press F1 for help. F2 saves to this browser. "
                "Use F3 to reopen a saved document after reloading the page.",
                "LICENSE"),
    "opengl-triangle": ("OpenGL Triangle", "Draw a triangle with the sample's vertex and fragment shaders.",
                        "The WebGL 2 canvas shows the result. Press a key to finish.", "LICENSE"),
    "asteroids": ("Asteroids", "Fly a ship through an animated asteroid field.",
                  "Use the arrow keys to steer and Space to fire.", "LICENSE"),
    "asylum": ("Asylum", "Explore an abandoned building in a text adventure.",
               "Enter the letter shown for an action. Enter x to quit.", "LICENSE"),
    "guess": ("Number Guess", "Find the hidden number before your guesses run out.",
              "Enter a number between 0 and 20 when prompted.", "LICENSE"),
    "rps": ("Rock Paper Scissors", "Play a scored match against the computer.",
            "Enter r, p or s for each round.", "LICENSE"),
    "tictactoe": ("Tic-Tac-Toe", "Play X against the computer on a numbered board.",
                  "Enter the number of an empty square to make your move.", "LICENSE"),
    "lijstjes": ("Lijstjes", "Open a keyboard-driven editor for lists and notes.",
                "Press F1 for help or Tab to switch panes. Ctrl+S saves the lists in this browser.", "LICENSE"),
    "lin-regressor": ("Lin-Regressor", "Calculate and plot a linear regression from your own data points.",
                      "Use a keyboard. Press Enter to start, 2 to enter points, and 5 to plot the fit. "
                      "Save datasets and results in this browser using the program's prompts.", "Lin-Regressor/gpl-3.0"),
    "fbjson": ("fbJson", "Build a JSON object and print it in the browser terminal.",
               "The program prints a serialized object with strings, numbers and an array.", "COPYING"),
    "fbjson-tests": ("fbJson Tests", "Run the library's parser and serializer checks.",
                     "The program prints the result of each check.", "COPYING"),
    "toybasic": ("ToyBASIC", "Evaluate your own arithmetic expressions with a small BASIC interpreter.",
                 "Try numbers, decimals, parentheses and the four arithmetic operators.", "LICENSE"),
    "kiwi-url": ("Kiwi URL Inspector", "Split an HTTP or HTTPS address into its parts with Kiwi's URL class.",
                 "Paste an address to see its protocol, host and resource. The tool makes no request to the address.",
                 "LICENSE"),
    "opensesh-mod": ("OpenSesh MIDI to MOD", "Inspect a MIDI file and convert it to a four-channel ProTracker module.",
                     "Use the sample or open your own MIDI file. The report shows notes and the changes made for MOD export.",
                     "COPYING"),
    "darkcastle-editor": ("Dark Castle Map Editor", "Design an isometric room with the original Dark Castle editor.",
                           "Move the cursor with the arrow keys and use A or Z to raise or lower a floor cell. "
                           "Save and reopen maps in this browser or download the original map file.", "LICENSE"),
    "station922-static": ("Station922 Static Site Preview", "Serve files with Station922's portable HTTP handler.",
                          "Try GET or HEAD against the sample site, or open a local folder to check its files.", "LICENSE"),
    "brpg-map": ("BRPGEngine Map Converter", "Convert a compact BRPG map into the 32-bit map format used by the RPG loader.",
                 "Open a .map file or try the sample. The preview shows blocked cells and overlays.", "LICENSE"),
    "visualfb-align": ("VisualFBEditor Code Alignment", "Run VisualFBEditor's Format with Basis Word command on FreeBASIC source.",
                       "Select the lines to align, or leave the selection empty to use the whole source.", "license.txt"),
    "freebasic-compiler": ("FreeBASIC Compiler", "Run the upstream FreeBASIC compiler in your browser.",
                           "Preprocess a BASIC program or generate C for the JS target. You can include local .bi headers.",
                           "doc/gpl.txt"),
    "freebasic-ng-compiler": ("FreeBASIC-NG Compiler", "Run the FreeBASIC-NG compiler in your browser.",
                              "Preprocess a BASIC program or generate C for the JS target. You can include local .bi headers.",
                              "doc/gpl.txt"),
    "lbasic-frontend": ("L-BASIC Parser", "Run L-BASIC's parse-and-dump stage in your browser.",
                        "Edit or open a BASIC file to inspect its declarations and syntax tree. "
                        "LLVM code generation uses the native L-BASIC compiler.", "LICENSE"),
    "azdecrypt-substitution": ("AZdecrypt Substitution Solver", "Solve alphabetic substitution ciphers with AZdecrypt's English four-gram search.",
                                "Try the sample or paste ciphertext and run a search locally in your browser.", "LICENSE"),
    "pyromax-workshop": ("Pyromax Dax Engine Workshop", "Run and jump through two original courses in the Pyromax Dax engine.",
                         "Press Enter to start, use the arrow keys to move, Space to jump and Z to fire. "
                         "Reach the gate and press Enter to continue.", "LICENSE"),
    "oregon-trail": ("Oregon Trail", "Buy supplies for a text-based wagon journey.",
                     "Answer the opening questions and enter whole dollar amounts at the store. "
                     "The prompts tell you which number to enter for each action.", "COPYING"),
}

GRAPHICS_PROGRAMS = {
    "darkcastle", "pyromax-dax",
    "asteroids", "basicverse-island", "catfish", "chessbrain", "darkcastle-editor", "imageproc-grayscale", "lijstjes", "lin-regressor", "nebunoid", "opengl-triangle", "raylib-camera",
    "fbsound-eq", "griffon-legend", "ohrrpgce-game", "phoenix", "pyromax-workshop", "raylib-window", "rekenblad", "schrijf", "strawberrynes", "tiko", "tilengine-learning", "tilengine-mode7", "ugomoku",
}
INTERACTIVE_PROGRAMS = GRAPHICS_PROGRAMS | {"clumsier", "fbfrog", "fgconvert", "fbrt-format", "hackit", "kiwi-url",
                                          "openmidisesh", "opensesh-mod", "station922-static", "toybasic", "azdecrypt-substitution", "brpg-map",
                                          "visualfb-align", "freebasic-compiler", "freebasic-ng-compiler", "lbasic-frontend"}

PLAYER_HINTS = {
    "darkcastle": "Enter selects · Arrow keys move · Escape returns to the menu",
    "pyromax-dax": "Enter starts or continues · Arrows move · Space jumps · Z fires",
    "fbsound-eq": "1 Original · 2 Boost 750 Hz · 3 Cut 750 Hz",
    "griffon-legend": "Arrow keys move · Space attacks · Esc opens the menu",
    "phoenix": "Space starts · Arrows move or attack · H heals · M restores mana · C casts",
    "pyromax-workshop": "Enter starts or continues · Arrows move · Space jumps · Z fires",
    "tilengine-mode7": "Up and Down move · Left and Right turn · Z resets",
    "strawberrynes": "Hold the Right arrow to change the picture.",
}

TEXT_GAMES = {"asylum", "guess", "oregon-trail", "rps", "tictactoe"}
TEXT_APPS = {"clumsier", "hackit", "lijstjes", "rekenblad", "schrijf"}
COMMAND_TOOLS = {"fbfrog", "fgconvert", "freebasic-compiler", "freebasic-ng-compiler", "lbasic-frontend", "toybasic"}
COMPONENT_DEMOS = {
    "azdecrypt-substitution", "basicverse-island", "brpg-map", "ext-list", "fbjson", "fbjson-tests", "fbrt-format",
    "fbsound-eq", "imageproc-grayscale", "kiwi-url", "opengl-triangle", "raylib-camera", "raylib-window",
    "station922-static", "tilengine-learning", "tilengine-mode7", "visualfb-align",
}
NUMERIC_INPUT_PROGRAMS = {"guess", "tictactoe"}

VIEWPORT_GAMES = {
    "darkcastle", "pyromax-dax",
    "asteroids", "basicverse-island", "catfish", "chessbrain", "griffon-legend", "nebunoid",
    "ohrrpgce-game", "pcem-fb486-freedos", "phoenix", "prospector", "pyromax-workshop",
    "strawberrynes", "ugomoku",
}
GAME_ASSETS = {"game.css": "fb-corpus-game.css", "game.js": "fb-corpus-game.js"}

PROGRAM_NOTICES = {
    "fbsound-eq": ("SDL2-LICENSE.txt",),
    "basicverse-island": ("DejaVu-LICENSE.txt",),
    "tilengine-learning": ("Tilengine-LICENSE.txt", "Tilengine-SOURCE.txt"),
    "tilengine-mode7": ("SDL2-LICENSE.txt", "LibPNG-LICENSE.txt", "Zlib-LICENSE.txt"),
    "phoenix": ("raylib-LICENSE.txt",),
}

# The OHRRPGCE SCons build provides its own shell and corresponding source
# archive. Keep its original output names because the loader uses them.
STANDALONE_FILES = {
    "ohrrpgce-game": ("ohrrpgce-game.js", "ohrrpgce-game.wasm", "ohrrpgce-game.data", "fb_rtlib.js", "source.zip"),
}

CSS = """/* FreeBASIC browser corpus, site.css. Shared catalog and tool presentation. */
:root{color-scheme:dark;--text:#c0c0ff;--line:#646494;--panel:#101020;--focus:#ffdc99}
*{box-sizing:border-box}
body{background:#000;color:var(--text);font:1rem/1.45 Georgia,"Times New Roman",serif;margin:0}
main{max-width:72rem;margin:auto;padding:clamp(1rem,4vw,2.5rem);min-width:0}
h1,h2{line-height:1.15;margin:0 0 .65rem}
h1{font-size:clamp(1.85rem,4vw,2.5rem)}
h2{font-size:1.35rem}
p{max-width:72ch;margin:0 0 1rem}
a{color:inherit;text-underline-offset:.15em}
a:hover{color:#fff}
:focus-visible{outline:2px solid var(--focus);outline-offset:3px}
nav{margin:0 0 1rem}
section+section{margin-top:1.75rem}
.run,.console button,.binding-actions button{display:inline-block;padding:.5rem .85rem;
background:var(--panel);color:var(--text);border:1px solid var(--line);font:inherit}
.run{margin-bottom:1.25rem;font-weight:bold}
footer{border-top:1px solid var(--line);padding-top:.8rem}

/* These two images are shared by the FBXL package catalog and OMA. */
.catalog{display:grid;grid-template-columns:279px minmax(0,1fr);gap:clamp(1rem,3vw,2.5rem);
padding:.5rem;max-width:90rem}
.catalog main{width:100%;margin:0;padding:1.1rem 1rem 1rem 0}
.catalog-nav{margin:0;align-self:start}
.catalog-nav img{display:block;width:203px;height:129px}
.catalog-nav .menu-background{padding:1rem;background:url('/oma/graphics/menubackground.png') repeat-y left top}
.catalog-nav ul{margin:0 auto;padding:.85rem .5rem;list-style:none;text-align:center;width:min(100%,216px);
background:#000;border:1px solid var(--line)}
.catalog-nav li+li{margin-top:.4rem}
.catalog-nav [aria-current="page"]{font-weight:bold}
.catalog main ul{margin:0;padding:0;list-style:none;border:1px solid var(--line)}
.catalog main li{display:grid;grid-template-columns:minmax(0,15rem) minmax(0,1fr);background:var(--panel)}
.catalog main li+li{border-top:1px solid var(--line)}
.catalog main li h2,.catalog main li p{font-size:1rem;margin:0;padding:.55rem .8rem;overflow-wrap:anywhere}
.catalog main li p{border-left:1px solid var(--line);max-width:none}
@media(max-width:690px){
    .catalog{grid-template-columns:minmax(0,1fr);gap:.6rem}
    .catalog main{padding:.5rem}
    .catalog-nav{display:flex;align-items:center}
    .catalog-nav img{width:108px;height:auto}
    .catalog-nav .menu-background{flex:1;padding:.4rem;background:none}
    .catalog-nav ul{width:100%;text-align:left;border:0}
    .catalog main li{grid-template-columns:minmax(0,1fr)}
    .catalog main li h2{padding-bottom:0}
    .catalog main li p{border-left:0}
}

.window-player #canvas{max-width:100%;height:auto}
.console{height:100dvh;display:flex;flex-direction:column;gap:.65rem;
padding:clamp(.5rem,3dvh,1.5rem) clamp(.75rem,4vw,2rem)}
.console nav,.console h1,.console p{margin:0}
.console h1{font-size:clamp(1.5rem,5vw,3rem)}
.console pre{background:var(--panel);color:#eee;flex:1;min-height:0;overflow:auto;
margin:0;padding:.85rem;font:clamp(.82rem,2.5vw,1rem)/1.45 ui-monospace,monospace;
white-space:pre-wrap;overflow-wrap:anywhere}
.console form:not([hidden]){display:grid;align-items:center;gap:.4rem .6rem;grid-template-columns:minmax(0,1fr) auto}
.console label{grid-column:1/-1}
.console input{min-width:0;padding:.5rem;font:inherit;border:1px solid var(--line)}
.console button,.binding-actions button{cursor:pointer}
.console button:disabled{opacity:.6;cursor:default}
.binding-head{font-size:clamp(1.8rem,5vw,3rem)}
.binding-fields{display:grid;gap:1rem;grid-template-columns:repeat(auto-fit,minmax(min(100%,20rem),1fr))}
.binding-fields label{display:block;font-weight:600;margin-bottom:.35rem}
.binding-fields textarea{width:100%;min-height:15rem;resize:vertical;padding:.75rem;background:var(--panel);
color:#eee;border:1px solid var(--line);font:1rem/1.45 ui-monospace,monospace}
.binding-actions{display:flex;align-items:center;flex-wrap:wrap;gap:.75rem;margin:.75rem 0}
.binding-actions button:disabled{opacity:.6;cursor:default}
#binding-status{min-height:1.5em;margin-bottom:.5rem}
#binding-diagnostics{overflow:auto;white-space:pre-wrap;max-height:15rem;padding:.75rem;background:var(--panel)}
.editor-actions{display:flex;flex-wrap:wrap;align-items:center;gap:.5rem;margin:.65rem 0}
.editor-terminal{position:relative;background:#000;color:#fff;overflow-x:auto;min-height:19rem;font:1rem monospace}
.editor-terminal table{border:0}
.editor-terminal>table{min-width:80ch}
.editor-terminal td{font:1rem/1.2 monospace}
#editor-controls{min-inline-size:0}
.editor-status{margin:.5rem 0}
.editor-status:empty{display:none}
.editor-actions button,.editor-actions select,.editor-actions input{font:inherit}
.editor-actions input{max-width:100%;min-width:0}
"""


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def verified_file(root, relative, expected):
    path = (root / relative).resolve()
    if not path.is_relative_to(root):
        raise ValueError(f"file escapes {root}: {relative}")
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError(f"file changed after compilation: {path}")
    return data


def page(title, content, stylesheet):
    return (f'<!doctype html>\n<html lang="en">\n<meta charset="utf-8">\n'
            f'<meta name="viewport" content="width=device-width, initial-scale=1">\n'
            f'<title>{html.escape(title)}</title>\n<link rel="stylesheet" href="{stylesheet}">\n'
            f'<main>\n{content}\n</main>\n</html>\n<!-- end of index.html -->\n')


# Use the author's current project page when the archived GitHub URL has moved.
PROJECT_PAGES = {
    "azdecrypt-substitution": "https://github.com/doranchak/azdecrypt",
    "lijstjes": "https://sourceforge.net/projects/lijstjes/",
    "pcem-fb486-freedos": "https://github.com/countingpine/PCEM_FB486",
    "rekenblad": "https://sourceforge.net/projects/rekenblad/",
    "schrijf": "https://sourceforge.net/projects/schrijf/",
}
CATALOG_LIST = '<ul id="program-list">'
CATALOG_PAGES = {
    "index.html": ("FreeBASIC in the browser", "Showcase"),
    "text.html": ("Text games and applications", "Text programs"),
    "tools.html": ("Compilers and developer tools", "Compilers and tools"),
    "demos.html": ("Library and engine demos", "Technical demos"),
}


def catalog_card(slug, title, description, build_origin):
    upstream = PROJECT_PAGES.get(slug, build_origin)
    if re.fullmatch(r"https://github\.com/[\w.-]+/[\w.-]+", upstream):
        host = "GitHub"
    elif re.fullmatch(r"https://sourceforge\.net/projects/[\w-]+/", upstream):
        host = "SourceForge"
    else:
        raise ValueError(f"Unexpected upstream address for {slug}: {upstream}")
    return (f'<li><h2><a href="{slug}/play.html">{title}</a></h2><p>{description} '
            f'<a href="{html.escape(upstream, quote=True)}">{host}</a> · '
            f'<a href="{slug}/sources.html">Source and license</a></p></li>')


def project_link(slug, build_origin):
    upstream = PROJECT_PAGES.get(slug, build_origin)
    return f'<a href="{html.escape(upstream, quote=True)}">Upstream project</a>'


def legacy_launch_page(title, target="play.html"):
    # The old directory URL still works for bookmarks on any static host.
    url = html.escape(target, quote=True)
    return page(html.unescape(title), f'<p><a href="{url}">Open program</a></p>', "../site.css").replace(
        '<link rel="stylesheet"', f'<meta http-equiv="refresh" content="0; url={url}">\n<link rel="stylesheet"', 1)


def catalog_return(shell):
    return re.sub(r'<a href="\./">Back to (?:program|Prospector)</a>',
                  '<a href="../">Back to catalog</a>', shell)


def text_programs_section(text_shell):
    # Keep the old text catalog URL while making every entry visible on the home page.
    _, separator, groups = text_shell.partition("<section>")
    if not separator:
        raise ValueError("Could not find text program groups")
    return '<section id="text-programs"><h2>Text programs</h2>\n' + separator + groups.split("</main>", 1)[0] + "</section>\n"


def style_catalog(shell, filename, available):
    if '<body class="catalog">' in shell:
        return shell
    links = []
    for leaf, (_, label) in CATALOG_PAGES.items():
        if leaf in available:
            current = ' aria-current="page"' if leaf == filename else ""
            target = "./" if leaf == "index.html" else leaf
            links.append(f'<li><a href="{target}"{current}>{label}</a></li>\n')
    sidebar = ('<body class="catalog">\n<nav class="catalog-nav" aria-label="Catalog navigation">\n'
               '<img src="/oma/graphics/menuswirl.png" width="203" height="129" alt="">\n'
               '<div class="menu-background"><ul>\n'
               + "".join(links) + '<li><a href="https://deb.fbxl.net/">FreeBASIC packages</a></li>\n'
               '<li><a href="/oma/">One Man Army games</a></li>\n'
               '</ul></div>\n</nav>\n')
    shell = shell.replace('<nav><a href="./">Catalog</a></nav>\n', '', 1)
    shell = shell.replace('<p><a href="text.html">Text games and console examples</a></p>\n', '', 1)
    return shell.replace('<main>', sidebar + '<main>', 1).replace('</html>', '</body>\n</html>', 1)


def catalog_pages(cards):
    pages = {
        "index.html": [("", [card for slug, card in cards.items()
                            if slug not in TEXT_GAMES | TEXT_APPS | COMMAND_TOOLS | COMPONENT_DEMOS])],
        "text.html": [("Text games", [card for slug, card in cards.items() if slug in TEXT_GAMES]),
                      ("Editors and applications", [card for slug, card in cards.items() if slug in TEXT_APPS])],
        "tools.html": [("", [card for slug, card in cards.items() if slug in COMMAND_TOOLS])],
        "demos.html": [("", [card for slug, card in cards.items() if slug in COMPONENT_DEMOS])],
    }
    available = {"index.html"} | {leaf for leaf, groups in pages.items() if any(group for _, group in groups)}
    for leaf, groups in pages.items():
        if leaf not in available:
            continue
        title, _ = CATALOG_PAGES[leaf]
        content = f"<h1>{title}</h1>\n"
        if leaf == "index.html":
            content += "<p>Games and graphical applications compiled to WebAssembly with fbc-js.</p>\n"
        for heading, group in groups:
            if not group:
                continue
            if heading:
                content += f"<section><h2>{heading}</h2>\n"
            content += ("<ul>" if heading else CATALOG_LIST) + "\n" + "\n".join(group) + "\n</ul>\n"
            if heading:
                content += "</section>\n"
        yield leaf, style_catalog(page(title, content, "site.css"), leaf, available)


def style_player(slug, shell):
    if slug in VIEWPORT_GAMES:
        if '<body class="game-player">' in shell:
            return shell
        # Keep the shell's own Module setup, filenames and storage hooks.
        if '<body>' not in shell or '</head>' not in shell or '</canvas>' not in shell:
            raise ValueError(f"{slug} has no supported game shell")
        if 'name="viewport"' not in shell:
            shell = shell.replace('</head>', '<meta name="viewport" content="width=device-width, initial-scale=1">\n</head>', 1)
        shell = shell.replace('</head>', '<link rel="stylesheet" href="../game.css">\n</head>', 1)
        shell = shell.replace('</canvas>', '</canvas>\n<script src="../game.js"></script>', 1)
        return shell.replace('<body>', '<body class="game-player">', 1)
    if slug in GRAPHICS_PROGRAMS and 'href="../site.css"' not in shell:
        shell = shell.replace('</head>', '<link rel="stylesheet" href="../site.css">\n</head>', 1)
        return shell.replace('<body>', '<body class="window-player">', 1)
    return shell


def presentation_assets():
    yield "site.css", (CSS + "/* end of site.css */\n").encode("utf-8")
    for leaf, source in GAME_ASSETS.items():
        yield leaf, Path(__file__).with_name(source).read_bytes()


def console_player(title, slug):
    input_mode = ' inputmode="numeric"' if slug in NUMERIC_INPUT_PROGRAMS else ""
    follow = ' data-follow-output="true"' if slug in TEXT_GAMES else ""
    return ('<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n'
            '<meta name="viewport" content="width=device-width, initial-scale=1, interactive-widget=resizes-content">\n'
            f'<title>{html.escape(title)}</title>\n<link rel="stylesheet" href="../site.css">\n'
            f'</head>\n<body data-args="[]"{follow}>\n<main class="console">\n'
            '<nav><a href="./">Back to program</a> · <a href="play.html">Restart</a></nav>\n'
            f'<h1>{html.escape(title)}</h1>\n'
            '<p id="program-status" role="status">Loading program…</p>\n'
            '<pre id="console-output" tabindex="0" aria-label="Program output"></pre>\n'
            '<form id="console-form" hidden>\n<label for="console-answer">Your answer</label>\n'
            f'<input id="console-answer" type="text" autocomplete="off" autocapitalize="none" spellcheck="false"{input_mode}>\n'
            '<button type="submit" disabled>Send</button>\n</form>\n</main>\n'
            '<script src="../console.js"></script>\n'
            '<script async type="text/javascript" src="index.js"></script>\n'
            '</body>\n</html>\n<!-- end of play.html -->\n')


def binding_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">fbfrog Binding Generator</h1>
<form id="binding-form">
  <div class="binding-actions">
    <label for="binding-file">Open header</label>
    <input type="file" id="binding-file" accept=".h,.txt">
  </div>
  <div class="binding-fields">
    <div><label for="binding-source">C header</label>
      <textarea id="binding-source" spellcheck="false" autocapitalize="off" autocomplete="off">typedef struct Point {
    int x;
    int y;
} Point;

int point_distance(Point a, Point b);
</textarea>
      <div class="binding-actions"><button type="submit" id="binding-convert">Convert header</button></div>
      <p id="binding-status" role="status"></p>
      <pre id="binding-diagnostics" hidden aria-label="Diagnostics"></pre>
    </div>
    <div><label for="binding-output">FreeBASIC bindings</label>
      <textarea id="binding-output" readonly spellcheck="false"></textarea>
      <a id="binding-download" download="bindings.bi" hidden>Download .bi</a>
    </div>
  </div>
</form>
<script src="bindings.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def editor_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">Clumsier</h1>
<p>The editor opens <code>hello.bas</code>. Click the editor or Tab to it, then use the keyboard to edit.
F1 starts a file, F2 opens a saved file, and F3 saves.</p>
<fieldset id="editor-controls" disabled>
    <legend>Source files</legend>
    <div class="editor-actions">
        <button type="button" data-editor-key="F1">New</button>
        <button type="button" data-editor-key="F2">Open</button>
        <button type="button" data-editor-key="F3">Save</button>
    </div>
    <div class="editor-actions">
        <label for="editor-upload">Import .bas</label>
        <input id="editor-upload" type="file" accept=".bas">
    </div>
    <div class="editor-actions">
        <label for="editor-files">Saved files</label>
        <select id="editor-files"></select>
        <button id="editor-download" type="button">Download .bas</button>
    </div>
</fieldset>
<p id="editor-status" class="editor-status" role="status">Loading editor…</p>
<form id="editor-prompt" hidden>
    <div class="editor-actions">
        <label for="editor-answer">Editor prompt</label>
        <input id="editor-answer" required maxlength="48" autocomplete="off" spellcheck="false">
        <button type="submit">Send</button>
    </div>
</form>
<div id="termDiv" class="editor-terminal" tabindex="0" aria-label="Clumsier terminal"></div>
<canvas id="canvas" hidden></canvas>
<script src="editor.js"></script>
<script async src="index.js"></script>''', "../site.css").replace("end of index.html", "end of play.html")


def hackit_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">HACKIT</h1>
<form id="hackit-open">
    <fieldset>
        <legend>Open a document</legend>
        <div class="editor-actions">
            <label for="hackit-upload">Local ASCII file (.bas, .bi, .txt)</label>
            <input id="hackit-upload" type="file" accept=".bas,.bi,.txt">
        </div>
        <button type="submit">Open in editor</button>
        <p>Leave the file blank to open the sample. Use a physical keyboard to edit.</p>
    </fieldset>
</form>
<div class="editor-actions"><button id="hackit-download" type="button" disabled>Download saved file</button></div>
<p id="hackit-status" role="status"></p>
<div id="termDiv" class="editor-terminal" tabindex="0" aria-label="HACKIT terminal" hidden></div>
<canvas id="canvas" hidden></canvas>
<script src="hackit.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def converter_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">FlightGear Pilot Conversions</h1>
<style>
#converter{max-width:30rem}
#converter-operation,#converter-fields input{display:block;width:100%;font:inherit;padding:.4rem;margin:0 0 .75rem}
#converter-fields label{display:block}
#converter-result{display:block;margin-top:1rem;white-space:pre-line}
</style>
<form id="converter">
    <label for="converter-operation">Conversion</label>
    <select id="converter-operation">
        <option value="1">Altitude: meters to feet</option>
        <option value="2">Altitude: feet to meters</option>
        <option value="3">Distance: kilometers to miles</option>
        <option value="4">Distance: miles to kilometers</option>
        <option value="5">Angle: DMS to decimal degrees</option>
        <option value="6">Angle: decimal degrees to DMS</option>
        <option value="7">Simulator descent distance</option>
    </select>
    <div id="converter-fields"></div>
    <div class="binding-actions"><button id="converter-calculate" type="submit">Calculate</button></div>
    <output id="converter-result" role="status" aria-live="polite"></output>
</form>
<script src="converter.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def format_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">fbrtLib Format</h1>
<style>
#format-form{max-width:36rem}
#format-form label,#format-form input{display:block}
#format-form input{width:100%;font:inherit;padding:.4rem;margin:0 0 .75rem}
#format-result{min-height:3rem;max-height:20rem;overflow:auto;
  padding:.75rem;border:1px solid #aaa;background:#fffdf7;
  white-space:pre-wrap;overflow-wrap:anywhere}
</style>
<form id="format-form">
    <label for="format-value">Value</label>
    <input id="format-value" type="number" inputmode="decimal" step="any" value="1234.5" required>
    <label for="format-mask">Format mask</label>
    <input id="format-mask" type="text" spellcheck="false" maxlength="5002" value="#,##0.00">
    <div class="binding-actions"><button id="format-submit" type="submit">Format</button></div>
    <p id="format-status" role="status"></p>
</form>
<pre id="format-result" aria-label="Formatted output" tabindex="0"></pre>
<script src="format.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def toybasic_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">ToyBASIC</h1>
<style>
#toy-form{max-width:36rem}
#toy-form label{display:block;font-weight:600}
#toy-expression{display:block;width:100%;font:1rem monospace;padding:.65rem;margin:.3rem 0 .6rem}
#toy-result{display:block;font:1.3rem monospace;overflow-wrap:anywhere}
</style>
<form id="toy-form">
    <label for="toy-expression">Arithmetic expression</label>
    <input id="toy-expression" required maxlength="256" value="5+2*3" spellcheck="false" autocomplete="off" autocapitalize="off">
    <p>Use decimals, parentheses, and +, -, * or /.</p>
    <div class="binding-actions"><button id="toy-submit" type="submit">Evaluate</button></div>
    <p id="toy-status" role="status"></p>
    <output id="toy-result" for="toy-expression" aria-label="Result" aria-live="polite"></output>
</form>
<script src="toybasic.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def kiwi_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">Kiwi URL Inspector</h1>
<style>
#kiwi-form{max-width:42rem}
#kiwi-form label,#kiwi-result dt{display:block;font-weight:600}
#kiwi-address{display:block;box-sizing:border-box;width:100%;font:1rem monospace;padding:.65rem;margin:.3rem 0 .6rem}
#kiwi-result dd{margin:0 0 .75rem;font:1rem monospace;overflow-wrap:anywhere}
</style>
<form id="kiwi-form">
    <label for="kiwi-address">HTTP or HTTPS address</label>
    <input id="kiwi-address" required maxlength="4096" value="https://freebasic.net/wiki/DocToc"
           inputmode="url" spellcheck="false" autocomplete="off" autocapitalize="off">
    <div class="binding-actions"><button type="submit" disabled>Inspect URL</button></div>
    <p id="kiwi-status" role="status"></p>
</form>
<dl id="kiwi-result" aria-live="polite" hidden>
    <dt>Protocol</dt><dd id="kiwi-protocol"></dd>
    <dt>Host and optional port</dt><dd id="kiwi-host"></dd>
    <dt>Path, query and fragment</dt><dd id="kiwi-resource"></dd>
</dl>
<script src="kiwi.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def opensesh_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">OpenSesh MIDI to MOD</h1>
<p>Convert a Standard MIDI File to a four-channel ProTracker module. Files stay in your browser.</p>
<style>
#opensesh-form label{display:block;font-weight:600;margin-bottom:.3rem}
#opensesh-file{font:inherit;max-width:100%;margin-bottom:.7rem}
#opensesh-download[hidden]{display:none}
#opensesh-result{white-space:pre-wrap;overflow-wrap:anywhere;font:1rem/1.55 monospace}
</style>
<form id="opensesh-form">
    <label for="opensesh-file">MIDI file</label>
    <input id="opensesh-file" type="file" accept=".mid,.midi,audio/midi">
    <div class="binding-actions">
        <button id="opensesh-convert" type="submit" disabled>Convert file</button>
        <button id="opensesh-sample" type="button" disabled>Convert sample</button>
        <a id="opensesh-download" hidden>Download MOD</a>
    </div>
</form>
<p id="opensesh-status" role="status"></p>
<pre id="opensesh-result" aria-label="Conversion report" hidden></pre>
<script src="opensesh-sample.js" defer></script>
<script src="opensesh.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def imageproc_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1 class="binding-head">fb-imageproc Grayscale</h1>
<p>Images stay in this browser. Images larger than 1,600 pixels on either side are resized to fit.</p>
<style>
.imageproc-controls{display:flex;flex-wrap:wrap;gap:1rem;align-items:end}
.imageproc-controls label{display:block;font-weight:600}
.imageproc-controls :is(select,input,button){font:inherit;max-width:100%}
#imageproc-images:not([hidden]){display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,18rem),1fr));gap:1rem}
#imageproc-images figure{margin:0;min-width:0}
#imageproc-images figcaption{margin-bottom:.35rem;font-weight:600}
#imageproc-images canvas{display:block;width:100%;height:auto;background:#fff}
#imageproc-status{margin-top:1rem}
</style>
<div class="binding-actions imageproc-controls">
    <div><label for="imageproc-file">Open image</label>
        <input id="imageproc-file" type="file" accept="image/png,image/jpeg,image/webp,image/bmp,.bmp"></div>
    <button id="imageproc-sample" type="button">Use sample</button>
    <div><label for="imageproc-method">Method</label>
        <select id="imageproc-method" disabled>
            <option value="luma">Luma</option>
            <option value="lightness">Lightness</option>
            <option value="average">Average</option>
            <option value="value">Value</option>
        </select></div>
    <a id="imageproc-download" hidden>Download PNG</a>
</div>
<p id="imageproc-status" role="status"></p>
<div id="imageproc-images" hidden>
    <figure><figcaption>Original</figcaption><canvas id="imageproc-original" role="img" aria-label="Original image"></canvas></figure>
    <figure><figcaption>Grayscale</figcaption><canvas id="imageproc-result" role="img" aria-label="Grayscale image"></canvas></figure>
</div>
<script src="imageproc.js" defer></script>''', "../site.css").replace("end of index.html", "end of play.html")


def azdecrypt_player(title):
    sample = ("Zit gsr gwltkcqzgkn lzggr qz zit tfr gy q fqkkgv kgqr. Tctkn tctfofu zit atthtk vgxsr esodw "
              "zit lzqokl qfr ghtf zit zqss vofrgvl wtygkt zit lxf vtfz rgvf. Of zit lxddtk it egxsr itqk eiosrktf "
              "hsqnofu of zit cossqut wtsgv, wxz of vofztk zit gfsn lgxfr vql zit vofr egdofu qekgll zit yotsrl. "
              "Gf q estqk fouiz it vgxsr wkofu gxz iol fgztwgga qfr vkozt rgvf viqz it iqr lttf. It rktv zit liqht "
              "gy zit dggf qfr dqkatr zit wkouiztlz lzqkl, zitf dqrt q sozzst latzei zg ligv vioei vqn zit esgxrl "
              "vtkt dgcofu. Zit fgztwgga vql yxss gy ldqss rkqvoful qfr eqktyxs gwltkcqzogfl. Ntqkl sqztk zit "
              "eiosrktf ygxfr oz of q vggrtf wgb wtiofr q litsy of zit sowkqkn. Zitn eqkkotr oz gxzlort qfr sggatr "
              "xh qz zit lqdt lan.")
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1>AZdecrypt Substitution Solver</h1>
<p>Paste a cipher with 40 to 4,000 letters, or try the sample below. Longer passages give the English four-gram model more evidence.</p>
<style>
#azdecrypt-form {display:grid;gap:.75rem}
#azdecrypt-ciphertext {min-height:12rem;width:100%;box-sizing:border-box;resize:vertical;font:inherit}
#azdecrypt-form .search-options {display:flex;flex-wrap:wrap;gap:.75rem;align-items:end}
#azdecrypt-form input,#azdecrypt-form select {font:inherit}
#azdecrypt-form label,#azdecrypt-form input,#azdecrypt-form select {display:block}
#azdecrypt-candidate {white-space:pre-wrap;overflow-wrap:anywhere;min-height:3rem}
</style>
<form id="azdecrypt-form">
  <label for="azdecrypt-ciphertext">Ciphertext</label>
  <textarea id="azdecrypt-ciphertext" spellcheck="false" required>DEMO_CIPHER</textarea>
  <div class="search-options">
    <label>Restarts <select id="azdecrypt-effort"><option>1</option><option>2</option><option selected>3</option><option>4</option></select></label>
    <label>Seed <input id="azdecrypt-seed" type="number" min="0" max="1000000" value="31" required></label>
    <button id="azdecrypt-submit" type="submit">Solve cipher</button>
    <button id="azdecrypt-cancel" type="button" disabled>Cancel search</button>
  </div>
</form>
<p id="azdecrypt-status" role="status"></p>
<h2>Candidate plaintext</h2>
<pre id="azdecrypt-candidate"></pre>
<script src="azdecrypt.js" defer></script>'''.replace("DEMO_CIPHER", html.escape(sample)), "../site.css")


def compiler_player(title, parse=False, llvm=False):
    includes = '' if parse else '<label>Include files <input type="file" id="compiler-includes" accept=".bi,text/plain" multiple></label>'
    mode = '<input type="hidden" id="compiler-mode" value="lbasic-parse">' if parse else '''<label>Output
    <select id="compiler-mode"><option value="c">C for the JS target</option>
    <option value="preprocess">Preprocessed BASIC</option></select></label>'''
    if llvm:
        mode = '''<label>Output <select id="compiler-mode">
    <option value="lbasic-parse">Parse dump</option><option value="lbasic-ir">LLVM bitcode</option>
    </select></label>'''
    sample = ("DIM amount AS LONG\namount = 6 * 7\nPRINT amount\n" if parse else
              "#define Factor 3\nDim As Integer quantity = 7\nPrint quantity * Factor\n")
    content = '''<nav><a href="./">Back to program</a></nav>
<h1>COMPILER_TITLE</h1>
<style>
#compiler-form label,#compiler-form input,#compiler-form select{display:block;max-width:100%}
.compiler-controls{display:flex;flex-wrap:wrap;gap:.7rem;align-items:end;margin:.6rem 0}
#compiler-source,#compiler-output{display:block;min-height:11rem;width:100%;box-sizing:border-box;
  resize:vertical;font:1rem/1.45 monospace;tab-size:4;margin:.25rem 0 .75rem}
#compiler-download[hidden],#compiler-diagnostics:empty{display:none}
#compiler-diagnostics{white-space:pre-wrap;overflow-wrap:anywhere}
</style>
<form id="compiler-form"COMPILER_WORKER>
  <div class="compiler-controls">
    <label>Open BASIC source <input type="file" id="compiler-file" accept=".bas,.bi,text/plain"></label>
    COMPILER_INCLUDES
  </div>
  <label>Source <textarea id="compiler-source" spellcheck="false" wrap="off" required>COMPILER_SAMPLE</textarea></label>
  <div class="compiler-controls">
    COMPILER_MODE
    <button type="submit">COMPILER_ACTION</button>
    <button type="button" id="compiler-cancel" disabled>Cancel</button>
  </div>
</form>
<p id="compiler-status" role="status"></p>
<pre id="compiler-diagnostics"></pre>
<label id="compiler-text-output">COMPILER_RESULT <textarea id="compiler-output" readonly spellcheck="false" wrap="off"></textarea></label>
<a id="compiler-download" class="run" hidden>Download output</a>
<script src="COMPILER_SCRIPT" defer></script>'''
    return page(title, content.replace("COMPILER_TITLE", html.escape(title)).replace("COMPILER_SAMPLE", html.escape(sample))
                .replace("COMPILER_INCLUDES", includes).replace("COMPILER_MODE", mode)
                .replace("COMPILER_WORKER", ' data-compiler-worker="llvm/compiler-worker.js"' if llvm else "")
                .replace("COMPILER_SCRIPT", "llvm/compiler.js" if llvm else "compiler.js")
                .replace("COMPILER_ACTION", "Parse" if parse else "Compile")
                .replace("COMPILER_RESULT", "Parse dump" if parse else "Generated source"), "../site.css")


def visualfb_align_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1>VisualFBEditor Code Alignment</h1>
<style>
#visualfb-tools{display:flex;align-items:end;flex-wrap:wrap;gap:.65rem}
#visualfb-tools label,#visualfb-tools input{display:block}
#visualfb-tools input{max-width:100%}
.visualfb-editors{display:grid;grid-template-columns:repeat(auto-fit,minmax(min(100%,22rem),1fr));gap:1rem}
.visualfb-editors textarea{display:block;min-height:12rem;width:100%;box-sizing:border-box;resize:vertical;
  font:1rem/1.45 monospace;tab-size:4;margin:.25rem 0 .75rem}
#visualfb-download[hidden]{display:none}
</style>
<div id="visualfb-tools">
  <label>Open UTF-8 source <input id="visualfb-file" type="file" accept=".bas,.bi,.frm,text/plain"></label>
  <button id="visualfb-sample" type="button">Load sample</button>
  <button id="visualfb-align" type="button">Align source</button>
</div>
<p id="visualfb-status" role="status"></p>
<div class="visualfb-editors">
  <label>Source <textarea id="visualfb-source" spellcheck="false" wrap="off"></textarea></label>
  <label>Aligned source <textarea id="visualfb-result" readonly spellcheck="false" wrap="off"></textarea></label>
</div>
<a id="visualfb-download" class="run" hidden>Download aligned source</a>
<script src="visualfb-align.js" defer></script>''', "../site.css")


def brpg_map_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1>BRPGEngine Map Converter</h1>
<style>
#brpg-tools {display:flex;flex-wrap:wrap;gap:.75rem;align-items:end}
#brpg-tools label,#brpg-tools input {display:block}
#brpg-tools input {max-width:100%}
#brpg-preview {max-width:100%;height:auto;image-rendering:pixelated;background:#172e38}
#brpg-download[hidden] {display:none}
.brpg-legend {display:flex;flex-wrap:wrap;gap:.75rem;margin:.5rem 0}
.brpg-legend span::before {display:inline-block;width:.8rem;height:.8rem;margin-right:.3rem;
  background:var(--ink);content:""}
</style>
<div id="brpg-tools">
  <label>Compact map <input id="brpg-file" type="file" accept=".map,application/octet-stream"></label>
  <button id="brpg-sample" type="button">Load sample</button>
</div>
<p id="brpg-status" role="status"></p>
<canvas id="brpg-preview" aria-label="Preview of the map's ground and overlay layers" hidden></canvas>
<p class="brpg-legend"><span style="--ink:#739777">Walkable</span>
  <span style="--ink:#25485b">Blocked</span><span style="--ink:#f1c582">Overlay</span></p>
<p id="brpg-cell" aria-live="polite"></p>
<a id="brpg-download" class="run" hidden>Download 32-bit map</a>
<script src="brpg-map.js" defer></script>''', "../site.css")


def station922_player(title):
    return page(title, '''<nav><a href="./">Back to program</a></nav>
<h1>Station922 Static Site Preview</h1>
<p>Serve the sample site or choose a folder from this computer. Station922 processes the request in the browser.</p>
<style>
#station-form {display:grid;gap:.75rem;max-width:36rem}
#station-form label {display:grid;gap:.25rem}
#station-form input,#station-form select {font:inherit;max-width:100%;box-sizing:border-box}
#station-form button {justify-self:start}
#station-headers,#station-body {white-space:pre-wrap;overflow-wrap:anywhere;max-height:22rem;overflow:auto}
</style>
<form id="station-form">
  <label>Site folder (optional) <input id="station-folder" type="file" webkitdirectory></label>
  <label>Method <select id="station-method"><option>GET</option><option>HEAD</option></select></label>
  <label>Host <input id="station-host" value="preview.test" maxlength="255" required></label>
  <label>Path <input id="station-target" value="/" maxlength="2048" required></label>
  <button id="station-submit" type="submit">Send request</button>
</form>
<p id="station-status" role="status"></p>
<h2>Response headers</h2>
<pre id="station-headers"></pre>
<h2>Response body</h2>
<pre id="station-body"></pre>
<button id="station-download" type="button" disabled>Download body</button>
<script src="station922.js" defer></script>''', "../site.css")


def write_program(destination, corpus, build_path, build, case):
    slug = build["slug"]
    title, description, instructions, license_name = PROGRAMS[slug]
    origin = build["origin"].removesuffix(".git")
    if not re.fullmatch(r"https://github\.com/[\w.-]+/[\w.-]+", origin):
        raise ValueError(f"unexpected upstream address for {slug}")
    project = (corpus / build["project"]).resolve()
    if not project.is_relative_to(corpus):
        raise ValueError(f"unexpected project path for {slug}")
    program = destination / slug
    program.mkdir()
    artifacts = (build_path / "apps" / slug).resolve()
    standalone = STANDALONE_FILES.get(slug)
    leaves = standalone or (("index.js", "index.wasm", "index.data") if "index.data" in build["artifacts"]
                            else ("index.js", "index.wasm"))
    library_link = ""
    if spec := (case or {}).get("static_archive"):
        archive = f"lib{spec[0]}.a"
        leaves += (archive,)
        library_link = f' · <a href="{html.escape(archive)}">Download Wasm library</a>'
    for leaf in leaves:
        (program / leaf).write_bytes(verified_file(artifacts, leaf, build["artifacts"][leaf]))
    shell = verified_file(artifacts, "index.html", build["artifacts"]["index.html"]).decode("utf-8")
    if slug in {"darkcastle", "openmidisesh", "pyromax-dax"}:
        shell = shell.replace("<title>Emscripten-Generated Code</title>", f"<title>{html.escape(title)}</title>", 1)
    browser_files = ()
    if slug == "fbfrog":
        shell = binding_player(title)
        browser_files = (("fb-corpus-bindings.js", "bindings.js"), ("fb-corpus-bindings-worker.js", "binding-worker.js"))
    elif slug == "clumsier":
        shell = editor_player(title)
        browser_files = (("fb-corpus-clumsier.js", "editor.js"),)
    elif slug == "hackit":
        shell = hackit_player(title)
        browser_files = (("fb-corpus-hackit.js", "hackit.js"),)
    elif slug == "fgconvert":
        shell = converter_player(title)
        browser_files = (("fb-corpus-converter.js", "converter.js"),
                         ("fb-corpus-converter-worker.js", "converter-worker.js"))
    elif slug == "fbrt-format":
        shell = format_player(title)
        browser_files = (("fb-corpus-format.js", "format.js"),
                         ("fb-corpus-format-worker.js", "format-worker.js"))
    elif slug == "toybasic":
        shell = toybasic_player(title)
        browser_files = (("fb-corpus-toybasic.js", "toybasic.js"),
                         ("fb-corpus-toybasic-worker.js", "toybasic-worker.js"))
    elif slug == "kiwi-url":
        shell = kiwi_player(title)
        browser_files = (("fb-corpus-kiwi.js", "kiwi.js"),
                         ("fb-corpus-kiwi-worker.js", "kiwi-worker.js"))
    elif slug == "opensesh-mod":
        shell = opensesh_player(title)
        browser_files = (("fb-corpus-opensesh.js", "opensesh.js"),
                         ("fb-corpus-opensesh-worker.js", "opensesh-worker.js"),
                         ("fb-corpus-opensesh-sample.js", "opensesh-sample.js"))
    elif slug == "openmidisesh":
        if '<script async type="text/javascript" src="index.js"></script>' not in shell:
            raise ValueError("OpenMIDISeSh needs the Emscripten canvas shell")
        shell = shell.replace("</head>", """<style>
body{margin:0;background:#14131a;color:#e3e1ed}
body>div.emscripten:not(#status),body>hr,#termContainer{display:none}
#canvas{display:block;max-width:100%;height:auto;margin:auto}
#browser-files{font:1rem/1.4 sans-serif;padding:.5rem}
#browser-files a{color:#d3c4fb}
#browser-files label,#browser-files button{margin-inline-start:.5rem}
</style>
</head>""", 1)
        shell = shell.replace("<body>", """<body>
<nav id="browser-files" aria-label="Browser files">
  <a href="../">Back to catalog</a>
  <label>Import file <input id="midi-import" type="file" disabled></label>
  <label>Workspace file <select id="midi-files" disabled></select></label>
  <button id="midi-refresh" type="button" disabled>Refresh files</button>
  <button id="midi-download" type="button" disabled>Download</button>
  <span id="midi-file-status" role="status"></span>
</nav>""", 1)
        shell = shell.replace('<script async type="text/javascript" src="index.js"></script>',
                              '<script src="midi-files.js"></script>\n    <script async src="index.js"></script>', 1)
        browser_files = (("fb-corpus-openmidisesh-files.js", "midi-files.js"),)
    elif slug == "station922-static":
        shell = station922_player(title)
        browser_files = (("fb-corpus-station922.js", "station922.js"),
                         ("fb-corpus-station922-worker.js", "station922-worker.js"))
    elif slug == "brpg-map":
        shell = brpg_map_player(title)
        browser_files = (("fb-corpus-brpg-map.js", "brpg-map.js"),
                         ("fb-corpus-brpg-map-worker.js", "brpg-map-worker.js"))
        (program / "sample.map").write_bytes(verified_file(project, "sample.map", build["sources"]["sample.map"]))
    elif slug == "visualfb-align":
        shell = visualfb_align_player(title)
        browser_files = (("fb-corpus-visualfb-align.js", "visualfb-align.js"),
                         ("fb-corpus-visualfb-align-worker.js", "visualfb-align-worker.js"))
        (program / "sample.bas").write_bytes(verified_file(project, "sample.bas", build["sources"]["sample.bas"]))
    elif slug in {"freebasic-compiler", "freebasic-ng-compiler", "lbasic-frontend"}:
        shell = compiler_player(title, parse=slug == "lbasic-frontend")
        browser_files = (("fb-corpus-compiler.js", "compiler.js"),
                         ("fb-corpus-compiler-worker.js", "compiler-worker.js"))
    elif slug == "azdecrypt-substitution":
        shell = azdecrypt_player(title)
        browser_files = (("fb-corpus-azdecrypt.js", "azdecrypt.js"),
                         ("fb-corpus-azdecrypt-worker.js", "azdecrypt-worker.js"))
    elif slug == "imageproc-grayscale":
        shell = imageproc_player(title)
        browser_files = (("fb-corpus-imageproc.js", "imageproc.js"),
                         ("fb-corpus-imageproc-worker.js", "imageproc-worker.js"))
        sample = "res/test3.bmp"
        (program / "sample.bmp").write_bytes(verified_file(project, sample, build["sources"][sample]))
    elif slug in GRAPHICS_PROGRAMS:
        hint = " · " + html.escape(PLAYER_HINTS[slug]) if slug in PLAYER_HINTS else ""
        shell = shell.replace("<body>", '<body>\n<nav style="padding: 0.75rem 1rem">'
                              '<a href="./">Back to program</a>' + hint + '</nav>', 1)
        if slug in {"darkcastle-editor", "griffon-legend", "phoenix", "pyromax-workshop", "tilengine-mode7"}:
            max_width = {"phoenix": "1200px", "tilengine-mode7": "800px"}.get(slug, "640px")
            shell = shell.replace("</head>", """<style>
body{margin:0;background:#172322;color:#e5f1ed;font:1rem/1.5 system-ui,sans-serif}
nav{text-align:center}
nav a{color:inherit}
body>hr,#termContainer,body>div.emscripten:not(#status){display:none}
.emscripten_border{border:0!important}
#canvas{width:min(100%,CANVAS_MAX);height:auto;image-rendering:pixelated}
</style>
</head>""".replace("CANVAS_MAX", max_width), 1)
        if slug == "darkcastle-editor":
            shell = shell.replace("</nav>", """</nav>
<section style="max-width:640px;margin:0 auto 1rem;padding:0 1rem">
  <h1>Dark Castle Map Editor</h1>
  <p>Arrows move the cursor. A and Z change the floor. Press F2, then F4 and Enter to save.
     Press F2, then F3 to open a saved map. F1 shows the editor’s controls.</p>
  <details id="darkcastle-maps">
    <summary>Saved maps</summary>
    <label for="darkcastle-map-list">Map file</label>
    <select id="darkcastle-map-list"></select>
    <button id="darkcastle-download" type="button" disabled>Download map</button>
    <span id="darkcastle-map-status" role="status"></span>
  </details>
</section>
<script src="darkcastle.js" defer></script>""", 1)
            browser_files = (("fb-corpus-darkcastle.js", "darkcastle.js"),)
    else:
        shell = console_player(title, slug)
    for source, name in browser_files:
        shutil.copyfile(Path(__file__).with_name(source), program / name)
    (program / "play.html").write_text(catalog_return(style_player(slug, shell)), encoding="utf-8", newline="\n")

    # Ship the exact program inputs, including file data and applied options.
    # Record the input hashes at compile time so unrelated working-tree edits
    # cannot become part of the downloadable sources.
    notices = [Path(__file__).with_name("licenses") / leaf for leaf in PROGRAM_NOTICES.get(slug, ())]
    if slug == "fbrt-format":
        notices.append(Path(__file__).resolve().parents[1] / "doc" / "lgpl.txt")
    license_path = project / license_name
    source_notices = [verified_file(project, leaf, build["sources"][leaf]) for leaf in (case or {}).get("notices", [])]
    (program / "license.txt").write_bytes(b"\n\n".join([
        *(path.read_bytes() for path in (license_path, *notices)), *source_notices,
    ]))
    if not standalone:
        with ZipFile(program / "source.zip", "w", compression=ZIP_DEFLATED) as archive:
            for relative, expected in sorted(build["sources"].items()):
                base = Path(__file__).resolve().parent if case.get("adapter") and relative == case["entry"] else project
                archive.writestr(relative, verified_file(base, relative, expected))
            archive.write(license_path, license_name)
            for notice in notices:
                archive.write(notice, notice.name)
            for source, name in browser_files:
                archive.write(Path(__file__).with_name(source), "browser/" + name)
            archived_case = {key: value for key, value in case.items() if key != "adapter"}
            archive.writestr("fb-corpus-js.json", json.dumps([archived_case], indent=2) + "\n")
            archive.writestr("BUILD.txt", f"{title}\nUpstream: {origin}\nRevision: {build['revision']}\n\n"
                             "The archive contains this program's compilation inputs. Put them under a\n"
                             f"directory named {build['project']} and build using the included case and\n"
                             "build_scripts/fb-corpus-js.py from FreeBASIC with the fbc-js SDK.\n")
    parent = '<a href="../">Catalog</a>' if slug in INTERACTIVE_PROGRAMS else '<a href="../text.html">Text programs</a>'
    content = (f'<nav>{parent}</nav>\n<h1>{html.escape(title)}</h1>\n'
               f'<p>{html.escape(description)} {html.escape(instructions)}</p>\n'
               '<a class="run" href="play.html">Run in browser</a>\n'
               f'<footer>{project_link(slug, origin)} · '
               '<a href="source.zip">Download source</a> · <a href="license.txt">License</a>' + library_link + '</footer>')
    (program / "sources.html").write_text(page(title, content, "../site.css"), encoding="utf-8")
    (program / "index.html").write_text(legacy_launch_page(html.escape(title)), encoding="utf-8")
    return catalog_card(slug, html.escape(title), html.escape(description), origin)


def direct_catalog_changes(previous, old_sums, changes):
    """Upgrade catalog markup from before titles linked directly to play.html."""
    catalogs = {}
    visited = set()

    def convert_card(match):
        slug, title, description = match.groups()
        source_page = f"{slug}/sources.html"
        if source_page in old_sums:
            raise ValueError(f"Legacy card already has a source page: {slug}")
        old_page = f"{slug}/index.html"
        if old_page not in old_sums or f"{slug}/play.html" not in old_sums:
            raise ValueError(f"Catalog entry is missing its launch or details: {slug}")
        details = (previous / old_page).read_bytes().decode("utf-8")
        match_origin = re.search(r'<a href="(https://github\.com/[^"]+)">Upstream project</a>',
                                 details)
        origin = match_origin.group(1) if match_origin else ""
        if match_origin and slug in PROJECT_PAGES:
            details = details.replace(match_origin.group(), project_link(slug, origin), 1)
        changes[source_page] = details.encode("utf-8")
        changes[old_page] = legacy_launch_page(title).encode("utf-8")
        visited.add(slug)
        return catalog_card(slug, title, description, origin)

    for leaf in ("index.html", "text.html"):
        if leaf in old_sums:
            shell = changes[leaf].decode("utf-8")
            catalogs[leaf] = re.sub(r'<li><h2><a href="([\w-]+)/">([^<]+)</a></h2><p>([^<]+)</p></li>',
                                    convert_card, shell)
    if not visited:
        return
    expected = {relative.split("/")[0] for relative in old_sums if relative.count("/") == 1
                and relative.endswith("/play.html")}
    if visited != expected:
        raise ValueError(f"Legacy catalog launch links are incomplete: {sorted(expected - visited)}")
    prefix, _, content = catalogs["index.html"].partition("<main>")
    index = prefix + "<main>" + content.replace("<ul>", CATALOG_LIST, 1)
    if "text.html" in catalogs:
        index = index.replace("</main>", text_programs_section(catalogs["text.html"]) + "</main>", 1)
    catalogs["index.html"] = index
    for leaf, shell in catalogs.items():
        changes[leaf] = shell.encode("utf-8")


def restyle_site(previous, site, overlay):
    """Stage only presentation changes against a complete verified release."""
    previous, site, overlay = (folder.resolve() for folder in (previous, site, overlay))
    for destination in (site, overlay):
        if destination.exists() or destination.is_relative_to(previous):
            raise ValueError(f"Use a new stage directory outside the previous release: {destination}")
    if site.is_relative_to(overlay) or overlay.is_relative_to(site):
        raise ValueError("The stage and overlay must be separate directories")

    old_sums = {}
    for line in (previous / "SHA256SUMS.txt").read_text(encoding="ascii").splitlines():
        checksum, relative = line.split("  ", 1)
        source = (previous / relative).resolve()
        if not source.is_relative_to(previous) or relative in old_sums:
            raise ValueError(f"Invalid manifest path: {relative}")
        with source.open("rb") as file:
            if hashlib.file_digest(file, "sha256").hexdigest() != checksum:
                raise ValueError(f"Previous release file changed: {relative}")
        old_sums[relative] = checksum

    changes = dict(presentation_assets())
    has_text = "text.html" in old_sums
    for relative in old_sums:
        if relative in ("index.html", "text.html"):
            shell = (previous / relative).read_text(encoding="utf-8")
            available = {"index.html", "text.html"} if has_text else {"index.html"}
            changes[relative] = style_catalog(shell, relative, available).encode("utf-8")
        elif relative.endswith("/play.html"):
            shell = (previous / relative).read_text(encoding="utf-8")
            player = catalog_return(style_player(relative.split("/")[0], shell))
            if player != shell:
                changes[relative] = player.encode("utf-8")

    direct_catalog_changes(previous, old_sums, changes)
    next_sums = old_sums.copy()
    for relative, data in changes.items():
        next_sums[relative] = hashlib.sha256(data).hexdigest()
    manifest = "".join(f"{digest}  {leaf}\n" for leaf, digest in sorted(next_sums.items()))
    changes["SHA256SUMS.txt"] = manifest.encode("ascii")
    for relative in sorted(old_sums.keys() | changes.keys()):
        dest = site / relative
        dest.parent.mkdir(parents=True, exist_ok=True)
        if relative not in changes:
            # The preview only reads the old large downloads. Keep one copy on disk.
            os.link(previous / relative, dest)
            continue
        dest.write_bytes(changes[relative])
        if relative == "SHA256SUMS.txt" or old_sums.get(relative) != next_sums[relative]:
            dest = overlay / relative
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes(changes[relative])
    print(f"Staged {len(next_sums)} assets with presentation changes in {overlay}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("corpus", "profile", "build", "site"):
        parser.add_argument("--" + key, type=Path, required=key == "site")
    parser.add_argument("--restyle-from", type=Path, help="a verified release on the same volume as --site")
    parser.add_argument("--overlay", type=Path, help="new directory for changed files when restyling")
    parser.add_argument("--only", nargs="+", choices=sorted(PROGRAMS))
    args = parser.parse_args()
    if args.restyle_from:
        if not args.overlay or any((args.corpus, args.profile, args.build, args.only)):
            parser.error("use --restyle-from, --site, and --overlay together")
        restyle_site(args.restyle_from, args.site, args.overlay)
        return
    if not all((args.corpus, args.profile, args.build)) or args.overlay:
        parser.error("packaging requires --corpus, --profile, and --build; --overlay is for restyling")
    builds = {row["slug"]: row for row in read_json(args.build / "build-results.json")}
    checks = {row["slug"]: row for row in read_json(args.build / "browser-results.json")}
    profile = {case["slug"]: case for case in read_json(args.profile)}
    selected = args.only or list(PROGRAMS)
    for slug in selected:
        build, check = builds.get(slug, {}), checks.get(slug, {})
        if build.get("status") != "linked" or check.get("status") != "passed":
            parser.error(f"{slug} requires a successful build and browser check")
        if build["artifacts"] != check["artifacts"]:
            parser.error(f"{slug} changed after its browser check")
        if slug not in STANDALONE_FILES and slug not in profile:
            parser.error(f"{slug} requires a build profile")
    args.site.mkdir()
    items = {slug: write_program(args.site, args.corpus.resolve(), args.build, builds[slug], profile.get(slug))
             for slug in selected}
    for leaf, shell in catalog_pages(items):
        (args.site / leaf).write_text(shell, encoding="utf-8", newline="\n")
    for leaf, data in presentation_assets():
        (args.site / leaf).write_bytes(data)
    shutil.copyfile(Path(__file__).with_name("fb-corpus-console.js"), args.site / "console.js")
    sums = []
    for file in sorted(args.site.rglob("*")):
        if file.is_file():
            checksum = hashlib.sha256(file.read_bytes()).hexdigest()
            sums.append(f"{checksum}  {file.relative_to(args.site).as_posix()}")
    (args.site / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n", encoding="ascii", newline="\n")
    print(f"Packaged {len(items)} browser-verified programs in {args.site}")


if __name__ == "__main__":
    main()

# end of fb-corpus-js-site.py
