#!/usr/bin/env sh
UMICAT_LOG_FILE_PATH="../umicat.log"
TOTAL_REQUEST_COUNT="100"
TOTAL_REQUEST_COUNT_PORT_80="0"
TOTAL_REQUEST_COUNT_PORT_81="0"
TOTAL_REQUEST_COUNT_PORT_82="0"
TOTAL_REQUEST_COUNT_PORT_83="0"

echo "" > "${UMICAT_LOG_FILE_PATH}"
# service umicat restart
# wrk -t2 -c200 -d10s http://127.0.0.1:10201/
i=0
while [ "$i" -lt "${TOTAL_REQUEST_COUNT}" ]; do 
    curl http://127.0.0.1:10201/ > /dev/null 2>&1
    i=$((i+1))
done
while read -r LINE; do
    LINE=$(echo "${LINE}" | awk -F ' <-> ' '{print $2}' | awk -F ':' '{print $2}')
    case "${LINE}" in
    "80")
        TOTAL_REQUEST_COUNT_PORT_80=$((TOTAL_REQUEST_COUNT_PORT_80 + 1))
    ;;
    "81")
        TOTAL_REQUEST_COUNT_PORT_81=$((TOTAL_REQUEST_COUNT_PORT_81 + 1))
    ;;
    "82")
        TOTAL_REQUEST_COUNT_PORT_82=$((TOTAL_REQUEST_COUNT_PORT_82 + 1))
    ;;
    "83")
        TOTAL_REQUEST_COUNT_PORT_83=$((TOTAL_REQUEST_COUNT_PORT_83 + 1))
    ;;
    esac
done < "${UMICAT_LOG_FILE_PATH}"
TOTAL_REQUEST_COUNT=$((TOTAL_REQUEST_COUNT_PORT_80 + TOTAL_REQUEST_COUNT_PORT_81 + TOTAL_REQUEST_COUNT_PORT_82 + TOTAL_REQUEST_COUNT_PORT_83))
echo "Total requests: ${TOTAL_REQUEST_COUNT}."
echo "${TOTAL_REQUEST_COUNT_PORT_80} requests are forwarded to port 80."
echo "${TOTAL_REQUEST_COUNT_PORT_81} requests are forwarded to port 81."
echo "${TOTAL_REQUEST_COUNT_PORT_82} requests are forwarded to port 82."
echo "${TOTAL_REQUEST_COUNT_PORT_83} requests are forwarded to port 83."
