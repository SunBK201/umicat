# umicat
![umicat logo](docs/umicat_logo.png)

<p align="center">
<a href="https://github.com/SunBK201/umicat"><img src="https://hits.seeyoufarm.com/api/count/incr/badge.svg?url=https%3A%2F%2Fgithub.com%2FSunBK201%2Fumicat&count_bg=%2379C83D&title_bg=%23555555&icon=&icon_color=%23E7E7E7&title=umicat&edge_flat=false"/></a>
<a href="/LICENSE"><img src="https://img.shields.io/badge/license-BSD-green.svg" alt="license" /></a>
<img src="https://img.shields.io/badge/platform-linux-lightgrey" />
</p>

umicat is a L4 reverse proxy load balance server.

# Build and Install
```bash
make
sudo make install
```

# Usage

## Load Balance Policy
- `round_robin`: Round-robin load balance policy.
- `ip_hash`: IP hash load balance policy.
- `least_conn`: Least-connection load balance policy.

Please configure `/etc/umicat/umicat.conf` before you start using it:
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
        }
    ],
    "workers": 2,
    "log_level": "info",
    "log_file": "/var/log/umicat/umicat.log"
}
```

Then, you can enjoy it:
```bash
sudo umicat
```