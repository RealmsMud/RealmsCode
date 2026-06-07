#!/usr/bin/env bash
#
# api-smoke.sh - live smoke + concurrency soak for the REST API.
#
# Run from the host after publishing 8080 (docker/dev/dev-up.sh maps it) and starting
# the freshly-built server inside the sandbox (docker/dev/dev-server.sh):
#
#   STAFF_NAME=Bob STAFF_PW=secret ZONE=misc sh test/api-smoke.sh
#
# Env vars:
#   API_BASE          base URL                         (default http://localhost:8080)
#   STAFF_NAME/PW     a real staff login               (required for authed checks via login)
#   API_TOKEN         a pre-issued bearer token        (alternative to login; skips it)
#   ZONE              a zone to read from              (default misc)
#   BADSTAFF_PW       wrong password for STAFF_NAME    (default "definitely-wrong")
#   NONSTAFF_NAME/PW  a non-staff login                (asserts login 403 if set)
#   RUN_SOAK          set to 1 to run the concurrency soak (skipped by default)
#   SOAK_SECONDS      soak duration                    (default 15)
#   SOAK_CONCURRENCY  parallel soak workers            (default 20)
#
# Entity lists/details are self-deriving: it lists each type in ZONE and GETs the
# first id, so you don't pass ids. Uses python3 for JSON parsing.
set -u

API_BASE="${API_BASE:-http://localhost:8080}"
BADSTAFF_PW="${BADSTAFF_PW:-definitely-wrong}"
ZONE="${ZONE:-misc}"
RUN_SOAK="${RUN_SOAK:-}"
SOAK_SECONDS="${SOAK_SECONDS:-15}"
SOAK_CONCURRENCY="${SOAK_CONCURRENCY:-20}"
BODY=/tmp/api-smoke.body

pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  PASS  %s\n' "$1"; }
no()  { fail=$((fail+1)); printf '  FAIL  %s\n' "$1"; }

# status METHOD PATH [curl args...] -> prints HTTP code; body written to $BODY.
status() {
    method="$1"; path="$2"; shift 2
    curl -s -o "$BODY" -w '%{http_code}' -X "$method" "$@" "$API_BASE$path"
}

# expect EXPECTED METHOD PATH DESC [curl args...]
expect() {
    exp="$1"; method="$2"; path="$3"; desc="$4"; shift 4
    got="$(status "$method" "$path" "$@")"
    if [ "$got" = "$exp" ]; then ok "$desc ($method $path -> $got)"
    else no "$desc ($method $path -> got $got, want $exp)"; fi
}

auth() { printf 'Authorization: Bearer %s' "$1"; }
jlen()  { python3 -c "import json;print(len(json.load(open('$BODY'))))" 2>/dev/null; }
jfirst(){ python3 -c "import json;d=json.load(open('$BODY'));print(d[0]['id'] if d else '')" 2>/dev/null; }

# If a staff name was given without a password, prompt for it silently (keeps the
# secret out of shell history and argv). stty/read is portable across sh and bash.
if [ -n "${STAFF_NAME:-}" ] && [ -z "${STAFF_PW:-}" ] && [ -z "${API_TOKEN:-}" ] && [ -t 0 ]; then
    printf 'Staff password for %s: ' "$STAFF_NAME" >&2
    stty -echo 2>/dev/null; read -r STAFF_PW; stty echo 2>/dev/null
    printf '\n' >&2
fi

echo "== Public + unauthenticated =="
expect 200 GET  /api/version           "version is public"
grep -q '"version"' "$BODY" && ok "version body has version field" || no "version body missing version field"
# The list route is "/api/zones/" (trailing slash); Crow 301-redirects the slashless form.
expect 401 GET  /api/zones/            "zones list requires auth (middleware 401)"
expect 401 GET  /api/zones/misc        "zone detail requires auth"

echo "== Login matrix =="
expect 400 POST /api/auth/login "login rejects missing fields"   -H 'Content-Type: application/json' -d '{}'
if [ -n "${STAFF_NAME:-}" ]; then
    expect 401 POST /api/auth/login "login rejects bad password" \
        -H 'Content-Type: application/json' -d "{\"name\":\"$STAFF_NAME\",\"pw\":\"$BADSTAFF_PW\"}"
else
    echo "  SKIP  login bad-password (set STAFF_NAME)"
fi
if [ -n "${NONSTAFF_NAME:-}" ] && [ -n "${NONSTAFF_PW:-}" ]; then
    expect 403 POST /api/auth/login "login rejects non-staff" \
        -H 'Content-Type: application/json' -d "{\"name\":\"$NONSTAFF_NAME\",\"pw\":\"$NONSTAFF_PW\"}"
fi

TOKEN="${API_TOKEN:-}"
if [ -n "$TOKEN" ]; then
    ok "using supplied API_TOKEN (login skipped)"
elif [ -n "${STAFF_NAME:-}" ] && [ -n "${STAFF_PW:-}" ]; then
    code="$(status POST /api/auth/login -H 'Content-Type: application/json' \
            -d "{\"name\":\"$STAFF_NAME\",\"pw\":\"$STAFF_PW\"}")"
    if [ "$code" = "200" ]; then
        ok "staff login -> 200"
        TOKEN="$(grep -o '"token"[ ]*:[ ]*"[^"]*"' "$BODY" | sed 's/.*"\([^"]*\)"$/\1/')"
        [ -n "$TOKEN" ] && ok "login returned a token" || no "login body had no token"
    else
        no "staff login -> got $code, want 200 (check STAFF_NAME/STAFF_PW)"
    fi
else
    echo "  SKIP  authed checks (set STAFF_NAME/STAFF_PW or API_TOKEN)"
fi

if [ -n "$TOKEN" ]; then
    echo "== Authenticated reads (zone: $ZONE) =="
    expect 200 GET /api/zones/         "zones list"                  -H "$(auth "$TOKEN")"
    grep -q '{' "$BODY" && ok "zones list body is JSON" || no "zones list not JSON"
    expect 401 GET /api/zones/         "garbage token rejected"      -H 'Authorization: Bearer not.a.jwt'
    expect 404 GET /api/zones/__nope__ "missing zone -> 404"         -H "$(auth "$TOKEN")"
    expect 200 GET "/api/zones/$ZONE"  "zone detail"                 -H "$(auth "$TOKEN")"

    # Per entity type: list, then GET the first item's detail (self-deriving).
    for t in rooms objects monsters quests; do
        code="$(status GET "/api/zones/$ZONE/$t" -H "$(auth "$TOKEN")")"
        if [ "$code" != "200" ]; then no "$t list ($ZONE) -> got $code, want 200"; continue; fi
        ok "$t list ($ZONE) -> 200 ($(jlen) items)"
        fid="$(jfirst)"
        if [ -n "$fid" ]; then
            expect 200 GET "/api/zones/$ZONE/$t/$fid" "$t detail #$fid" -H "$(auth "$TOKEN")"
        else
            echo "  SKIP  $t detail ($ZONE has no $t)"
        fi
    done
    expect 404 GET "/api/zones/$ZONE/objects/32000"  "missing object -> 404"  -H "$(auth "$TOKEN")"
    expect 404 GET "/api/zones/$ZONE/monsters/32000" "missing monster -> 404" -H "$(auth "$TOKEN")"
    expect 404 GET "/api/zones/$ZONE/rooms/32000"    "missing room -> 404"    -H "$(auth "$TOKEN")"

    if [ -n "$RUN_SOAK" ] && [ "$RUN_SOAK" != "0" ]; then
        echo "== Concurrency soak (${SOAK_SECONDS}s x ${SOAK_CONCURRENCY} workers) =="
        soakdir="$(mktemp -d)"
        end=$(( $(date +%s) + SOAK_SECONDS ))
        i=0
        while [ "$i" -lt "$SOAK_CONCURRENCY" ]; do
            (
                bad=0; n=0
                while [ "$(date +%s)" -lt "$end" ]; do
                    # rotate the zones list and a per-zone entity list (index-served)
                    c="$(curl -s -o /dev/null -w '%{http_code}' -H "$(auth "$TOKEN")" "$API_BASE/api/zones/$ZONE/rooms")"
                    n=$((n+1))
                    case "$c" in 2*) : ;; *) bad=$((bad+1)) ;; esac
                done
                printf '%s %s\n' "$n" "$bad" > "$soakdir/$i"
            ) &
            i=$((i+1))
        done
        wait
        total=0; bados=0
        for f in "$soakdir"/*; do
            set -- $(cat "$f"); total=$((total + $1)); bados=$((bados + $2))
        done
        rm -rf "$soakdir"
        echo "  soak: $total requests, $bados non-2xx"
        [ "$bados" -eq 0 ] && ok "soak had zero non-2xx responses" || no "soak had $bados non-2xx responses"
        expect 200 GET /api/version "server still alive after soak"
    else
        echo "  SKIP  concurrency soak (set RUN_SOAK=1 to run)"
    fi
fi

echo
echo "==== $pass passed, $fail failed ===="
[ "$fail" -eq 0 ]
