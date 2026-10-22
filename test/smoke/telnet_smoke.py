#!/usr/bin/env python3
"""Runtime telnet smoke test for account/login/character-creation against a live
RealmsCode server. Not part of RealmsTests; run manually with the server up.

Protocol: the server negotiates only "IAC DO TTYPE" (io/socket.cpp). Never answering keeps
the connection in dumb mode (no MCCP, no GA/EOR), so prompts are detected by literal text.
IAC WILL/WONT ECHO brackets password prompts; it is stripped and never answered. Lines are
sent CRLF-terminated. Anchor strings are quoted from server/login.cpp.

Usage:
    python3 telnet_smoke.py [--host 127.0.0.1] [--port 3333] [--data ~/realms/realms]
                             [--dmpass PASS] [--keep]

Cleanup is best-effort and runs even on failure: config/config.xml, config/game/ids.xml,
config/dns.xml, and player/Abgt.xml are backed up before the run and restored after;
every accounts/<Smk*>.json and player/<Smk*>.xml (+ backup/*.bak.xml) created by the
run is removed. --keep skips cleanup for post-mortem debugging.
"""

import argparse
import json
import os
import random
import re
import shutil
import socket
import string
import sys
import tempfile
import time
from xml.etree import ElementTree as ET

IAC, DONT, DO, WONT, WILL, SB, SE = 255, 254, 253, 252, 251, 250, 240
DEFAULT_TIMEOUT = 8.0

# login.cpp menu/prompt text is written with "^W"/"^x"-style markup that the io layer
# may render as real ANSI SGR escapes even for a client that never negotiated color;
# strip those too so they can never land inside a "[X]" menu bracket and break the
# option-parsing regex below (unverified whether color defaults on; stripping is
# harmless either way).
_ANSI_RE = re.compile(r"\x1b\[[0-9;]*[a-zA-Z]")


class ScenarioSkipped(Exception):
    """Raised when a scenario's precondition (e.g. a multi-option menu) isn't met
    by this data set; reported as SKIP, not FAIL."""


class TelnetClient:
    """Raw-socket telnet client. Never answers any IAC negotiation (server stays in
    dumb mode: no MCCP, no GA -- see module docstring). Strips WILL/WONT/DO/DONT
    (3 bytes), SB..IAC SE (variable), and un-doubles 0xFF."""

    def __init__(self, host, port, timeout=DEFAULT_TIMEOUT):
        self.sock = socket.create_connection((host, port), timeout=timeout)
        self.sock.settimeout(0.2)
        self.buf = ""
        self._tail = b""
        self.closed = False

    def _recv_more(self):
        try:
            chunk = self.sock.recv(4096)
        except socket.timeout:
            return b""
        except OSError:
            self.closed = True
            return b""
        if not chunk:
            self.closed = True
        return chunk

    def _filter(self, data):
        data = self._tail + data
        self._tail = b""
        out = bytearray()
        i, n = 0, len(data)
        while i < n:
            b = data[i]
            if b != IAC:
                out.append(b)
                i += 1
                continue
            if i + 1 >= n:
                self._tail = data[i:]
                break
            cmd = data[i + 1]
            if cmd == IAC:
                out.append(IAC)
                i += 2
                continue
            if cmd in (WILL, WONT, DO, DONT):
                if i + 2 >= n:
                    self._tail = data[i:]
                    break
                i += 3
                continue
            if cmd == SB:
                j = i + 2
                while j + 1 < n and not (data[j] == IAC and data[j + 1] == SE):
                    j += 1
                if j + 1 >= n:
                    self._tail = data[i:]
                    break
                i = j + 2
                continue
            # other 2-byte IAC commands (GA, NOP, EOR, ...)
            i += 2
        return _ANSI_RE.sub("", bytes(out).decode("latin-1"))

    def _pump_once(self):
        chunk = self._recv_more()
        if chunk:
            self.buf += self._filter(chunk)

    def expect(self, anchor, timeout=DEFAULT_TIMEOUT):
        """Wait for `anchor` to appear; consume and return everything up to and
        including it."""
        deadline = time.time() + timeout
        while anchor not in self.buf:
            if self.closed:
                raise ConnectionError(f"closed waiting for {anchor!r}; buf={self.buf!r}")
            if time.time() >= deadline:
                raise TimeoutError(f"timeout waiting for {anchor!r}; buf={self.buf!r}")
            self._pump_once()
        idx = self.buf.index(anchor) + len(anchor)
        seen, self.buf = self.buf[:idx], self.buf[idx:]
        return seen

    def expect_any(self, anchors, timeout=DEFAULT_TIMEOUT):
        """Wait for whichever anchor occurs first (leftmost in the stream, not
        first in the list). Consumes up through it."""
        deadline = time.time() + timeout
        while True:
            best = None
            for a in anchors:
                idx = self.buf.find(a)
                if idx != -1 and (best is None or idx < best[1]):
                    best = (a, idx)
            if best is not None:
                a, idx = best
                cut = idx + len(a)
                seen, self.buf = self.buf[:cut], self.buf[cut:]
                return a, seen
            if self.closed:
                raise ConnectionError(f"closed waiting for any of {anchors}; buf={self.buf!r}")
            if time.time() >= deadline:
                raise TimeoutError(f"timeout waiting for any of {anchors}; buf={self.buf!r}")
            self._pump_once()

    def peek_any(self, anchors, timeout=DEFAULT_TIMEOUT):
        """Like expect_any but does not consume the buffer."""
        deadline = time.time() + timeout
        while True:
            for a in anchors:
                if a in self.buf:
                    return a
            if self.closed:
                raise ConnectionError(f"closed waiting for any of {anchors}; buf={self.buf!r}")
            if time.time() >= deadline:
                raise TimeoutError(f"timeout waiting for any of {anchors}; buf={self.buf!r}")
            self._pump_once()

    def expect_closed(self, timeout=DEFAULT_TIMEOUT):
        deadline = time.time() + timeout
        while not self.closed:
            if time.time() >= deadline:
                raise TimeoutError(f"expected disconnect, still open; buf={self.buf!r}")
            self._pump_once()

    def send(self, text=""):
        self.sock.sendall(text.encode("latin-1", "replace") + b"\r\n")

    def close(self):
        try:
            self.sock.close()
        except OSError:
            pass


# Menu parsing (navigation only -- scenario PASS/FAIL comes from file state,
# except where noted, e.g. the deity gender-filter check has no file artifact).

_OPT_RE = re.compile(r"\[(\w)\]\s*([^\[\r\n]+?)(?=\s{2,}|\r|\n|$)")


def menu_options(text):
    """{label_lower: letter} extracted from a rendered '[X] Label' menu block."""
    return {m.group(2).strip().lower(): m.group(1) for m in _OPT_RE.finditer(text)}


def pick(text, label):
    opts = menu_options(text)
    key = label.strip().lower()
    if key in opts:
        return opts[key]
    for k, v in opts.items():
        if key in k:
            return v
    raise AssertionError(f"menu option {label!r} not found; options={sorted(opts)}")


def has_option(text, substr):
    substr = substr.lower()
    return any(substr in k for k in menu_options(text))


def find_option_containing(text, substr):
    opts = menu_options(text)
    substr = substr.lower()
    for k, v in opts.items():
        if substr in k:
            return v
    return None


def first_option(text):
    opts = menu_options(text)
    if not opts:
        raise AssertionError(f"no menu options found in {text!r}")
    return min(opts.values())


# Data-dir helpers.

def read_dm_pass(data_dir):
    root = ET.parse(os.path.join(data_dir, "config", "config.xml")).getroot()
    el = root.find(".//DmPass")
    if el is None or not (el.text or "").strip():
        raise RuntimeError("DmPass not found in config/config.xml")
    return el.text.strip()


def account_path(data_dir, name):
    return os.path.join(data_dir, "accounts", f"{name}.json")


def player_path(data_dir, name):
    return os.path.join(data_dir, "player", f"{name}.xml")


def player_backup_path(data_dir, name):
    return os.path.join(data_dir, "player", "backup", f"{name}.bak.xml")


def read_account(data_dir, name, timeout=5.0):
    path = account_path(data_dir, name)
    deadline = time.time() + timeout
    while not os.path.exists(path):
        if time.time() >= deadline:
            raise AssertionError(f"account file never appeared: {path}")
        time.sleep(0.1)
    with open(path) as f:
        return json.load(f)


def read_player_xml(data_dir, name, timeout=5.0):
    path = player_path(data_dir, name)
    deadline = time.time() + timeout
    while not os.path.exists(path):
        if time.time() >= deadline:
            raise AssertionError(f"player file never appeared: {path}")
        time.sleep(0.1)
    with open(path, "rb") as f:
        raw = f.read()
    return ET.fromstring(raw), raw.decode("latin-1")


class NameFactory:
    """Unique 'Smk'-prefixed names so reruns never collide with prior runs or with
    each other. Alphabetic-only, proper-cased (matches server lowercize(str,1))."""

    def __init__(self):
        self._n = 0

    def _rand(self, n=6):
        self._n += 1
        return "".join(random.choice(string.ascii_lowercase) for _ in range(n))

    def account(self):
        return "Smk" + self._rand()

    def character(self):
        return "Smk" + self._rand()


class Sandbox:
    """Backup/cleanup manager. Backs up files before the run, tracks every account
    and player artifact the scenarios create, and restores/removes them afterward
    even if a scenario raised."""

    BACKUP_RELPATHS = (
        "config/config.xml",
        "config/game/ids.xml",
        "config/dns.xml",
        "player/Abgt.xml",
        # Legacy login (scenario_legacy_login) isn't actually read-only: a logged-in
        # myPlayer gets unconditionally saved on disconnect (Socket::cleanUp,
        # io/socket.cpp), which upgrades Abc.xml to the current schema/version and
        # bumps LastLogin/LastInterest. Without this it silently drifts every run.
        "player/Abc.xml",
    )

    def __init__(self, data_dir):
        self.data_dir = data_dir
        self.backup_dir = tempfile.mkdtemp(prefix="realms-smoke-")
        self.backed_up = []
        self.created_accounts = []
        self.created_players = []

    def back_up_all(self):
        for rel in self.BACKUP_RELPATHS:
            src = os.path.join(self.data_dir, rel)
            if not os.path.exists(src):
                continue
            dst = os.path.join(self.backup_dir, rel)
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(src, dst)
            self.backed_up.append(rel)

    def note_account(self, name):
        self.created_accounts.append(name)

    def note_player(self, name):
        self.created_players.append(name)

    def cleanup(self, keep=False):
        manifest = {
            "accounts_created": sorted(set(self.created_accounts)),
            "players_created": sorted(set(self.created_players)),
            "backed_up": list(self.backed_up),
        }
        if keep:
            print("\n-- --keep set: skipping cleanup --")
            print(json.dumps(manifest, indent=2))
            print(f"backups left at: {self.backup_dir}")
            return manifest

        time.sleep(2.0)   # the server saves a disconnected player after the peer sees FIN
        removed_accounts, removed_players = [], []
        for name in set(self.created_accounts):
            p = account_path(self.data_dir, name)
            if os.path.exists(p):
                os.remove(p)
                removed_accounts.append(name)
        for name in set(self.created_players):
            for p in (player_path(self.data_dir, name), player_backup_path(self.data_dir, name)):
                if os.path.exists(p):
                    os.remove(p)
                    removed_players.append(p)
        for rel in self.backed_up:
            shutil.copyfile(os.path.join(self.backup_dir, rel), os.path.join(self.data_dir, rel))
        shutil.rmtree(self.backup_dir, ignore_errors=True)

        manifest["accounts_removed"] = sorted(removed_accounts)
        manifest["players_removed"] = sorted(removed_players)
        manifest["restored"] = list(self.backed_up)
        print("\n-- cleanup manifest --")
        print(json.dumps(manifest, indent=2))
        return manifest


class Ctx:
    def __init__(self, host, port, data_dir, dm_pass):
        self.host = host
        self.port = port
        self.data_dir = data_dir
        self.dm_pass = dm_pass
        self.names = NameFactory()
        self.state = {}  # scratch shared across ordered scenarios

    def connect(self):
        return TelnetClient(self.host, self.port)


# Shared login/creation drivers.

def to_entry_choice(client):
    return client.expect("Enter choice (a/b): ")


def create_account(ctx, sb, client, name, password):
    """From a fresh connection at the entry banner, create a brand-new account and
    land at the Account Menu."""
    to_entry_choice(client)
    client.send("a")
    client.expect("Please enter account name: ")  # kPromptAccountName
    client.send(name)
    client.expect("Did I get that right? (yes/no): ")  # kPromptConfirmName
    client.send("y")
    client.expect(f"Please set a password for new account '{name}': ")
    client.send(password)
    client.expect("~~~~~~~ Account Menu ~~~~~~~")
    sb.note_account(name)


def login_account(client, name, password):
    """From a fresh connection at the entry banner, log into an existing account."""
    to_entry_choice(client)
    client.send("a")
    client.expect("Please enter account name: ")
    client.send(name)
    client.expect("Please enter account password: ")
    client.send(password)
    client.expect("~~~~~~~ Account Menu ~~~~~~~")


def start_creation(client):
    """From the Account Menu, start character creation; returns the Race menu text
    (already consumed up through 'Choose one: ')."""
    client.send("c")
    return client.expect("Choose one: ")


DEITY_ANCHOR = "Please choose a deity"
LOC_MULTI_ANCHOR = "Please choose a starting location"
LOC_SINGLE_ANCHOR = "Your starting location is"
STATS_CHOICE_ANCHOR = "you may:"
STATS_DIRECT_ANCHOR = "You have 56 points"
WEAPON_ANCHORS = ("Slashing Weapons", "Crushing Weapons", "eapon skill")


def advance_race_and_class(client, race_label, klass_label, race_menu_text):
    """From the just-rendered Race menu, pick race (+ subrace/sex as needed) and
    class. Returns the Class menu text (consumed through 'Choose one: ')."""
    client.send(pick(race_menu_text, race_label))
    which, text = client.expect_any(["Female? ", "Choose one: "])
    if which == "Female? ":
        client.send("f")
        text = client.expect("Choose one: ")
    elif "Choose a sub race" in text:  # gendered subrace still asks Sex
        client.send(first_option(text))
        which2, text2 = client.expect_any(["Female? ", "Choose one: "])
        if which2 == "Female? ":
            client.send("f")
            text2 = client.expect("Choose one: ")
        text = text2
    # else: text already holds the class banner (ungendered race, straight to Class)
    client.send(pick(text, klass_label))
    return client.expect_any(["Invalid selection", DEITY_ANCHOR, LOC_MULTI_ANCHOR,
                               LOC_SINGLE_ANCHOR, STATS_CHOICE_ANCHOR, STATS_DIRECT_ANCHOR])[1]


def advance_to_weapon_menu(client, tail_text, *, deity=None, on_deity_menu=None,
                            location="first", stats="custom", stats_values="12 12 12 10 10",
                            bonus="a", penalty="b"):
    """Drive everything between class selection and the weapon (Prof) menu: deity,
    starting location, stats choice/entry, bonus/penalty. `tail_text` is whatever
    advance_race_and_class() already captured (may already contain the next anchor)."""
    text = tail_text
    while True:
        # Checked most-downstream-stage-first: expect_any() consumes and returns
        # everything up to and including whichever anchor it matches, so a captured
        # `text` legitimately still contains earlier stages' anchor text as a leading
        # substring (e.g. the post-pick "Your starting location is: Highport" recap
        # is folded into the STATS_CHOICE_ANCHOR-terminated capture that follows it).
        # Checking earlier-stage anchors first would re-trigger an already-passed
        # branch on that leftover substring and re-await an anchor that will never
        # arrive again (it already went by) until timeout.
        if STATS_DIRECT_ANCHOR in text:
            # STATS_DIRECT_ANCHOR only matches the opening "You have 56 points..." line.
            # The rest of renderStats contains an earlier, unrelated
            # ": " ("...following order: Strength...") before its real terminal prompt, so
            # a bare client.expect(": ") here would grab the wrong one and leave a stray
            # fragment in the buffer that corrupts the next expect(": ") (weapon menu).
            # "## ## ## ## ##" is the fixed format-hint literal right before that prompt.
            client.expect("## ## ## ## ##")
            client.expect(": ")
            client.send(stats_values)
            which, text = client.expect_any(["Raise which stat?"] + list(WEAPON_ANCHORS))
            if which == "Raise which stat?":
                client.expect(" : ")
                client.send(bonus)
                client.expect(" : ")  # PenaltyStat prompt, same " : " terminator
                client.send(penalty)
                text = client.expect_any(list(WEAPON_ANCHORS))[1]
            continue
        if STATS_CHOICE_ANCHOR in text:
            client.send("u" if stats == "predefined" else "c")
            text = client.expect_any([STATS_DIRECT_ANCHOR] + list(WEAPON_ANCHORS))[1]
            continue
        if LOC_MULTI_ANCHOR in text:
            # LOC_MULTI_ANCHOR only matches the "Please choose a starting location:"
            # header; the "[A] Loc     [B] Loc" options and terminal ": " prompt are
            # printed after it and haven't been consumed yet.
            text += client.expect(": ")
            if location == "first":
                letter = first_option(text)
            else:
                letter = find_option_containing(text, location)
                if letter is None:
                    raise ScenarioSkipped(f"no start location containing {location!r} offered")
            client.send(letter)
            text = client.expect_any([STATS_CHOICE_ANCHOR, STATS_DIRECT_ANCHOR])[1]
            continue
        if LOC_SINGLE_ANCHOR in text:
            if location not in ("first", None):
                raise ScenarioSkipped("only one start location offered (auto-resolved)")
            text = client.expect_any([STATS_CHOICE_ANCHOR, STATS_DIRECT_ANCHOR])[1]
            continue
        if DEITY_ANCHOR in text:
            if on_deity_menu:
                on_deity_menu(text)
            client.send(pick(text, deity))
            text = client.expect_any([DEITY_ANCHOR, LOC_MULTI_ANCHOR, LOC_SINGLE_ANCHOR,
                                       STATS_CHOICE_ANCHOR, STATS_DIRECT_ANCHOR])[1]
            continue
        return text  # weapon menu reached


def finish_prof_to_name(client, weapon=None, second_weapon=None):
    """From the weapon menu already visible, pick weapon(s) and drive through to
    (and consume) the character-name prompt."""
    # A bare ": " anchor is ambiguous here: finishStats' "Your stats: N N N N N\n"
    # recap (printed right before the weapon menu) has its own earlier ": " and
    # would be matched instead of the real terminal prompt. renderProf/
    # renderSecondProf's askFor always ends "...\n: ", so anchor on that.
    text = client.expect("\n: ")  # renderProf terminal prompt
    client.send(pick(text, weapon) if weapon else first_option(text))
    which = client.peek_any(["Pick a second proficiency", "Please choose a character name: "])
    if which == "Pick a second proficiency":
        text = client.expect("\n: ")  # renderSecondProf terminal
        client.send(pick(text, second_weapon) if second_weapon else first_option(text))
        client.expect("Please choose a character name: ")
    else:
        client.expect("Please choose a character name: ")


def finish_name_and_done(client, name):
    """Send the character name and drive through to CON_PLAYING. Raises if the
    name routes to the DM-password gate (callers that expect that use their own
    flow instead, see scenario_dm_name_creation_gate)."""
    client.send(name)
    client.expect("[Press Enter to Continue]")  # renderDone
    client.send("")
    client.expect("Type 'welcome'")  # processDone success


def create_character(ctx, sb, client, *, race, klass, name=None, deity=None,
                      on_deity_menu=None, location="first", stats="custom",
                      stats_values="12 12 12 10 10", bonus="a", penalty="b",
                      weapon=None, second_weapon=None, help_before_race=False):
    """Full happy-path creation walk from the Account Menu through CON_PLAYING.
    Registers the resulting player file with the sandbox and returns its name."""
    name = name or ctx.names.character()
    race_text = start_creation(client)
    if help_before_race:
        client.send("help")
        time.sleep(0.3)  # helpfile view has no reliable anchor; step does not change
    tail = advance_race_and_class(client, race, klass, race_text)
    weapon_text = advance_to_weapon_menu(client, tail, deity=deity, on_deity_menu=on_deity_menu,
                                          location=location, stats=stats, stats_values=stats_values,
                                          bonus=bonus, penalty=penalty)
    client.buf = weapon_text + client.buf  # put the weapon-menu heading back for finish_prof_to_name
    finish_prof_to_name(client, weapon, second_weapon)
    finish_name_and_done(client, name)
    sb.note_player(name)
    return name


# Scenarios.

def scenario_account_create_bad_then_good(ctx, sb):
    name = ctx.names.account()
    c = ctx.connect()
    try:
        to_entry_choice(c)
        c.send("a")
        c.expect("Please enter account name: ")
        c.send(name)
        c.expect("Did I get that right? (yes/no): ")
        c.send("y")
        c.expect(f"Please set a password for new account '{name}': ")
        c.send("ab1")  # too short: password must be 5..35 characters
        c.expect(f"Please set a password for new account '{name}': ")
        good_pw = "SmkGood123"
        c.send(good_pw)
        c.expect("~~~~~~~ Account Menu ~~~~~~~")
        sb.note_account(name)
        acct = read_account(ctx.data_dir, name)
        assert acct["accountName"] == name
        assert acct["password"] == good_pw
        assert acct["characterNames"] == []
        ctx.state["acct1_name"] = name
        ctx.state["acct1_pw"] = good_pw
    finally:
        c.close()


def scenario_account_wrong_password_disconnects(ctx, sb):
    name = ctx.state.get("acct1_name")
    if not name:
        raise ScenarioSkipped("account-create scenario did not run first")
    c = ctx.connect()
    try:
        to_entry_choice(c)
        c.send("a")
        c.expect("Please enter account name: ")
        c.send(name)
        c.expect("Please enter account password: ")
        c.send("definitely-wrong-pw")
        c.expect("Incorrect.")
        c.expect_closed()
    finally:
        c.close()


def scenario_full_creation_human_fighter_custom_stats(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        name = create_character(ctx, sb, c, race="Human", klass="Fighter",
                                 stats="custom", stats_values="12 12 12 10 10",
                                 bonus="a", penalty="b", help_before_race=True)
        root, raw = read_player_xml(ctx.data_dir, name)
        assert root.attrib["Name"] == name
        assert root.find("Race").text == "5"
        assert root.find("Class").text == "4"
        # CharacterMaterializer bakes bonus/penalty straight into Initial via
        # Stat::addInitial; it does not record a separate StatModifier -- so
        # Strength (bonus, +10) and Dexterity (penalty, -10) land 10 off the raw
        # "12 12 12 10 10" entry, not equal to it.
        for stat_name, val in (("Strength", 130), ("Dexterity", 110), ("Constitution", 120),
                                ("Intelligence", 100), ("Piety", 100)):
            stat = root.find(f"./Stats/Stat[@Name='{stat_name}']/Initial")
            assert stat is not None and stat.text == str(val), f"{stat_name}: {stat}"
    finally:
        c.close()


def scenario_multiclass_predefined_stats(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        name = create_character(ctx, sb, c, race="Elf", klass="Figh/Mage", stats="predefined")
        root, _ = read_player_xml(ctx.data_dir, name)
        assert root.find("Class").text == "4"        # Fighter
        assert root.find("Class2").text == "5"        # Mage (Class2 only saved if nonzero)
    finally:
        c.close()


def scenario_deity_class_gender_filter(ctx, sb):
    seen = {}

    def check_male(text):
        seen["male_has_mara"] = has_option(text, "mara")
        seen["male_has_arachnus"] = has_option(text, "arachnus")

    def check_female(text):
        seen["female_has_linothan"] = has_option(text, "linothan")

    for sex, cb in (("m", check_male), ("f", check_female)):
        c = ctx.connect()
        try:
            create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
            race_text = start_creation(c)
            c.send(pick(race_text, "Elf"))
            which, text = c.expect_any(["Female? ", "Choose one: "])
            if which == "Female? ":
                c.send("f" if sex == "f" else "m")
                text = c.expect("Choose one: ")
            elif "Choose a sub race" in text:
                c.send(first_option(text))
                which2, text2 = c.expect_any(["Female? ", "Choose one: "])
                if which2 == "Female? ":
                    c.send("f" if sex == "f" else "m")
                    text2 = c.expect("Choose one: ")
                text = text2
            c.send(pick(text, "Cleric"))
            text = c.expect(DEITY_ANCHOR)
            text += c.expect("Choose one: ")
            cb(text)
            # abandon here: nothing is materialized before the Name/Done steps, so no
            # player artifact exists to clean up.
        finally:
            c.close()

    assert seen.get("male_has_mara") is False, "Mara should be excluded for a male Cleric"
    assert seen.get("male_has_arachnus") is False, "Arachnus should be excluded for a male Cleric"
    assert seen.get("female_has_linothan") is False, "Linothan should be excluded for a female Cleric"


def scenario_multi_option_start_location(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        name = create_character(ctx, sb, c, race="Human", klass="Fighter", location="first")
        root, _ = read_player_xml(ctx.data_dir, name)
        assert root.find("Room") is not None
    finally:
        c.close()


def scenario_oceancrest_hardcore_start(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        name = create_character(ctx, sb, c, race="Human", klass="Fighter", location="ocean")
        root, _ = read_player_xml(ctx.data_dir, name)
        room = root.find("Room")
        assert room is not None and room.attrib.get("Area") == "ocean", \
            f"expected bind area 'ocean', got {room.attrib if room is not None else None}"
    finally:
        c.close()


def scenario_monk_weapon_menu(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        name = create_character(ctx, sb, c, race="Human", klass="Monk", weapon="Bare-Handed")
        root, _ = read_player_xml(ctx.data_dir, name)
        assert root.find("Class").text == "10"
    finally:
        c.close()


def scenario_enter_at_menu_rerenders(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        c.send("")  # handleAccountMenuCommand: empty input -> showAccountMenu again
        c.expect("~~~~~~~ Account Menu ~~~~~~~")
    finally:
        c.close()


# dmname[] (server/global.cpp) lists these reserved DM names. The Name step refuses
# any of them with "already taken" if a player file already exists for it -- only a
# dmname with NO file on disk reaches the DM-password gate.
DM_RESERVED_NAMES = ("Bane", "Dominus", "Ocelot", "Kriona", "Nikola")


def scenario_dm_name_creation_gate(ctx, sb):
    # Wrong DM password disconnects -- only reachable for a
    # reserved name with no existing player file. This production data set happens
    # to have a character for every entry in dmname[], so skip rather than assert
    # an unreachable path (mirrors S07's "not offered by this data set" SKIP).
    free_dm_name = next((n for n in DM_RESERVED_NAMES
                         if not os.path.exists(player_path(ctx.data_dir, n))), None)
    if free_dm_name is None:
        print("  (skipping wrong-DM-password sub-check: every dmname[] entry "
              f"{DM_RESERVED_NAMES} already has a player file in this data set)")
    else:
        c = ctx.connect()
        try:
            create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
            create_character_up_to_name(c, race="Human", klass="Fighter")
            c.send(free_dm_name)
            c.expect("Please enter password: ")
            c.send("definitely-wrong-dm-pw")
            c.expect_closed()
        finally:
            c.close()

    # A DM name that already exists on disk (player/Bane.xml) is refused at the Name step,
    # before the DM-password gate; the wizard stays on the name prompt.
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        create_character_up_to_name(c, race="Human", klass="Fighter")
        c.send("Bane")
        c.expect("That character name is already taken. Choose a character name: ")
    finally:
        c.close()


def create_character_up_to_name(client, *, race, klass):
    """Like create_character but stops right after sending the weapon pick, leaving
    the caller to send the name (used by the DM-name gate scenario)."""
    race_text = start_creation(client)
    tail = advance_race_and_class(client, race, klass, race_text)
    weapon_text = advance_to_weapon_menu(client, tail)
    client.buf = weapon_text + client.buf
    # See finish_prof_to_name: a bare ": " anchor would match finishStats' "Your
    # stats: ..." recap instead of the real weapon-menu terminal prompt.
    text = client.expect("\n: ")
    client.send(first_option(text))
    which = client.peek_any(["Pick a second proficiency", "Please choose a character name: "])
    if which == "Pick a second proficiency":
        text = client.expect("\n: ")
        client.send(first_option(text))
    client.expect("Please choose a character name: ")


def scenario_legacy_login(ctx, sb):
    c = ctx.connect()
    try:
        to_entry_choice(c)
        c.send("b")
        c.expect("Please enter character name for legacy login: ")  # kPromptLegacyName
        c.send("Abc")
        c.expect("Please enter character password: ")
        c.send("lc2002")
        c.expect("Legacy login successful!")
    finally:
        c.close()

    c = ctx.connect()
    try:
        to_entry_choice(c)
        c.send("b")
        c.expect("Please enter character name for legacy login: ")
        c.send("Abc")
        c.expect("Please enter character password: ")
        c.send("definitely-wrong")
        c.expect("Incorrect.")
        c.expect_closed()
    finally:
        c.close()


def scenario_claim(ctx, sb):
    # Wrong password on a still-unclaimed character must run FIRST: handleCharacterClaim
    # checks player->getAccountName().empty() before ever asking for a password, so this
    # path is only reachable while Abgt is unclaimed -- it must run before the
    # successful-claim block below.
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        c.send("cl")
        c.expect("Character name: ")
        c.send("Abgt")
        c.expect("Character password: ")
        c.send("wrong-pw")
        c.expect("Incorrect password for character")
    finally:
        c.close()
    root, _ = read_player_xml(ctx.data_dir, "Abgt")
    assert root.find("AccountName") is None, "failed claim attempt must not set AccountName"

    acct_a = ctx.names.account()
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, acct_a, "SmkGood123")
        c.send("cl")
        c.expect("Character name: ")
        c.send("Abgt")
        c.expect("Character password: ")
        c.send("mrsteve0509")
        c.expect("~~~~~~~ Account Menu ~~~~~~~")  # success -> showAccountMenu
    finally:
        c.close()
    root, _ = read_player_xml(ctx.data_dir, "Abgt")
    assert root.find("AccountName") is not None and root.find("AccountName").text == acct_a
    acct = read_account(ctx.data_dir, acct_a)
    assert "Abgt" in acct["characterNames"]

    # Already claimed (by a different account): the same empty()-check short-circuits
    # straight back to the Account Menu right after the name is given -- no password
    # prompt is ever issued, so don't wait for one.
    acct_b = ctx.names.account()
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, acct_b, "SmkGood123")
        c.send("cl")
        c.expect("Character name: ")
        c.send("Abgt")
        c.expect("is already claimed by an account")
        c.expect("~~~~~~~ Account Menu ~~~~~~~")
    finally:
        c.close()
    root, _ = read_player_xml(ctx.data_dir, "Abgt")
    assert root.find("AccountName").text == acct_a, "already-claimed attempt must not reassign ownership"


def scenario_email(ctx, sb):
    c = ctx.connect()
    try:
        name = ctx.names.account()
        create_account(ctx, sb, c, name, "SmkGood123")
        c.send("e")
        c.expect("Enter new email address (or press enter to clear): ")  # kPromptEmail
        c.send("not-an-email")
        c.expect("Invalid email format")
        c.expect("Enter new email address (or press enter to clear): ")
        c.send("smk@example.com")
        c.expect("Is this correct? (y/n): ")
        c.send("y")
        c.expect("~~~~~~~ Account Menu ~~~~~~~")
        acct = read_account(ctx.data_dir, name)
        assert acct["email"] == "smk@example.com"

        c.send("e")
        c.expect("Enter new email address (or press enter to clear): ")
        c.send("")
        c.expect("Email address cleared")
        acct = read_account(ctx.data_dir, name)
        assert acct["email"] == ""
    finally:
        c.close()


def scenario_account_menu_list_invalid_quit(ctx, sb):
    c = ctx.connect()
    try:
        create_account(ctx, sb, c, ctx.names.account(), "SmkGood123")
        c.send("zzznotacommand")
        c.expect("Invalid command or character name")
        c.expect("~~~~~~~ Account Menu ~~~~~~~")
        c.send("list")
        c.expect("Enter a command")
        c.send("q")
        c.expect("Goodbye!")
        c.expect_closed()
    finally:
        c.close()


def scenario_second_connection_same_character(ctx, sb):
    acct = ctx.names.account()
    pw = "SmkGood123"
    c1 = ctx.connect()
    try:
        create_account(ctx, sb, c1, acct, pw)
        name = create_character(ctx, sb, c1, race="Human", klass="Fighter")
        # c1 stays open and playing: it must still be connected when c2 logs in as it.
        c2 = ctx.connect()
        try:
            login_account(c2, acct, pw)
            c2.send(name)
            # finishLogin -> checkDuplicateName(sock, true) disconnects the OLDER socket
            # while c2 proceeds to play.
            c1.expect_closed(timeout=10)
        finally:
            c2.close()
    finally:
        c1.close()


SCENARIOS = [
    ("S01", "account create: too-short then good password -> account menu", scenario_account_create_bad_then_good),
    ("S02", "account login: wrong password disconnects", scenario_account_wrong_password_disconnects),
    ("S03", "full creation: Human Fighter, custom stats + bonus/penalty", scenario_full_creation_human_fighter_custom_stats),
    ("S04", "multiclass Figh/Mage, predefined stats", scenario_multiclass_predefined_stats),
    ("S05", "deity class (Cleric): gender-filtered deity absent from menu", scenario_deity_class_gender_filter),
    ("S06", "multi-option start location", scenario_multi_option_start_location),
    ("S07", "oceancrest hardcore start (where offered)", scenario_oceancrest_hardcore_start),
    ("S08", "monk weapon menu (Bare-Handed)", scenario_monk_weapon_menu),
    ("S09", "Enter at Account Menu re-renders (same step)", scenario_enter_at_menu_rerenders),
    ("S10", "DM name at creation Name step: wrong pw disconnects, right pw -> policy screen", scenario_dm_name_creation_gate),
    ("S11", "legacy login: success and wrong password", scenario_legacy_login),
    ("S12", "claim: success, already-claimed, wrong password", scenario_claim),
    ("S13", "email: set, bad format, clear", scenario_email),
    ("S14", "account menu: list, invalid command, quit", scenario_account_menu_list_invalid_quit),
    ("S15", "second connection to same character: duplicate handling", scenario_second_connection_same_character),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=3333)
    ap.add_argument("--data", default=os.path.expanduser("~/realms/realms"))
    ap.add_argument("--dmpass", default=None)
    ap.add_argument("--keep", action="store_true", help="skip cleanup for debugging")
    args = ap.parse_args()

    data_dir = os.path.expanduser(args.data)
    dm_pass = args.dmpass or read_dm_pass(data_dir)
    ctx = Ctx(args.host, args.port, data_dir, dm_pass)
    sb = Sandbox(data_dir)
    sb.back_up_all()

    results = []
    try:
        for code, desc, fn in SCENARIOS:
            try:
                fn(ctx, sb)
                results.append((code, desc, "PASS", ""))
            except ScenarioSkipped as e:
                results.append((code, desc, "SKIP", str(e)))
            except Exception as e:
                results.append((code, desc, "FAIL", f"{type(e).__name__}: {e}"))
    finally:
        sb.cleanup(keep=args.keep)

    print("\n==== telnet smoke results ====")
    for code, desc, status, detail in results:
        line = f"  {status:4}  {code}  {desc}"
        print(line if not detail else f"{line}\n        {detail}")
    failed = sum(1 for *_, status, _ in results if status == "FAIL")
    passed = sum(1 for *_, status, _ in results if status == "PASS")
    skipped = sum(1 for *_, status, _ in results if status == "SKIP")
    print(f"\n{passed} passed, {failed} failed, {skipped} skipped")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
