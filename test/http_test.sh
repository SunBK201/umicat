#!/usr/bin/env sh
UMICAT_LOG_FILE_PATH="/var/log/umicat/umicat.log"
TOTAL_REQUEST_COUNT="0"
TOTAL_REQUEST_COUNT_PORT_80="0"
TOTAL_REQUEST_COUNT_PORT_81="0"

echo "" > "${UMICAT_LOG_FILE_PATH}"
service umicat restart
wrk -t2 -c200 -d10s http://127.0.0.1:10201
while read -r LINE; do
    LINE=$(echo "${LINE}" | awk -F ' <-> ' '{print $2}' | awk -F ':' '{print $2}')
    case "${LINE}" in
    "80")
        TOTAL_REQUEST_COUNT_PORT_80=$((TOTAL_REQUEST_COUNT_PORT_80 + 1))
    ;;
    "81")
        TOTAL_REQUEST_COUNT_PORT_81=$((TOTAL_REQUEST_COUNT_PORT_81 + 1))
    ;;
    esac
done < "${UMICAT_LOG_FILE_PATH}"
TOTAL_REQUEST_COUNT=$((TOTAL_REQUEST_COUNT_PORT_80 + TOTAL_REQUEST_COUNT_PORT_81))
echo "Total requests: ${TOTAL_REQUEST_COUNT}."
echo "${TOTAL_REQUEST_COUNT_PORT_80} requests are forwarded to port 80."
echo "${TOTAL_REQUEST_COUNT_PORT_81} requests are forwarded to port 81."