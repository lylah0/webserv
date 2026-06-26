#!/bin/bash
# Reproduit le test du cgi_tester :
#   "multiple workers(20) doing multiple times(5): Post on /directory/youpi.bla with size 100000000"
#
# Usage : ./stress_test.sh [URL] [FICHIER]
# Exemple : ./stress_test.sh http://localhost:8080/directory/youpi.bla payload_100mb.bin

URL="${1:-http://localhost:8080/directory/youpi.bla}"
PAYLOAD="${2:-payload_100mb.bin}"
WORKERS=5
TIMES=20

if [ ! -f "$PAYLOAD" ]; then
    echo "Fichier introuvable : $PAYLOAD"
    echo "Génère-le avec : head -c 100000000 /dev/zero | tr '\\0' 'A' > $PAYLOAD"
    exit 1
fi

echo "=== Stress test ==="
echo "URL        : $URL"
echo "Payload    : $PAYLOAD ($(wc -c < "$PAYLOAD") octets)"
echo "Workers    : $WORKERS en parallèle"
echo "Itérations : $TIMES par worker"
echo "==================="

# Un worker = TIMES requêtes séquentielles
worker() {
    local id=$1
    for i in $(seq 1 $TIMES); do
        # -s silencieux, -o /dev/null jette le corps de réponse,
        # -w affiche le code HTTP, --fail-with-body pour voir les erreurs
        code=$(curl -s -v -o /dev/null -w "%{http_code}" \
                    --max-time 60 \
                    --fail-with-body \
                    -X POST \
                    --data-binary "@$PAYLOAD" \
                    "$URL" 2>/tmp/curl_err_${id}_${i})
        rc=$?
        if [ $rc -ne 0 ]; then
            echo "[worker $id iter $i] ECHEC curl (rc=$rc) : $(cat /tmp/curl_err_${id}_${i})"
        else
            echo "[worker $id iter $i] HTTP $code"
        fi
    done
}

# Lance les 20 workers en parallèle
for w in $(seq 1 $WORKERS); do
    worker $w &
done

# Attend que tous finissent
wait
echo "=== Terminé ==="