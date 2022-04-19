# umicat
umicat is a TCP/UDP reverse proxy load balance server.

# Build
```bash
make
```

# Usage

## Load Balance Policy
- `round_robin`: Round-robin load balance policy.
- `ip_hash`: IP hash load balance policy.
- `least_conn`: Least-connection load balance policy.

Please configure `conf/umicat.conf` before you start using it:
```bash
{
    "mode": "tcp",
    "localport": 10201,
    "policy": "round_robin",
    "upstream": [
        {
            "upstream_ip": "127.0.0.1",
            "upstream_port": 80,
            "weight": 2
        },
        {
            "upstream_ip": "127.0.0.1",
            "upstream_port": 81,
            "weight": 2
        },
        {
            "upstream_ip": "127.0.0.1",
            "upstream_port": 82,
            "weight": 2
        },
        {
            "upstream_ip": "127.0.0.1",
            "upstream_port": 83,
            "weight": 2
        }
    ],
    "workers": 2,
    "log_level": "info",
    "log_file": "umicat.log"
}
```

Then, you can enjoy it:
```bash
sudo ./umicat -c conf/umicat.conf
```