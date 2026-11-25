# -*- coding: utf-8 -*-
"""清理远端权限探测文件，并核对仓库结构。"""
import json
import sys
import urllib.request
import urllib.error

TOK = sys.argv[1]
REPO = "Hbur-by/Robotic-Manipulator-Applications-in-Garment-Care"
BASE = "https://api.github.com"


def api(url, method="GET", data=None):
    req = urllib.request.Request(url, method=method)
    req.add_header("Authorization", "Bearer " + TOK)
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("User-Agent", "rmagc")
    if data is not None:
        req.add_header("Content-Type", "application/json")
        req.data = json.dumps(data).encode()
    try:
        with urllib.request.urlopen(req, timeout=30) as r:
            raw = r.read().decode()
            return r.status, json.loads(raw) if raw else {}
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode()


def main():
    st, body = api(f"{BASE}/repos/{REPO}/contents/_probe_perm.txt")
    if st == 200:
        sha = body["sha"]
        st2, _ = api(
            f"{BASE}/repos/{REPO}/contents/_probe_perm.txt",
            "DELETE",
            {"message": "chore: 移除权限探测文件", "sha": sha, "branch": "main"},
        )
        print("delete probe:", st2)
    else:
        print("probe file absent:", st)

    st, body = api(f"{BASE}/repos/{REPO}/contents/")
    print("root listing:", st)
    if st == 200:
        for it in body:
            print("  -", it["name"], it["type"])

    for path in ["code_v0.1", "code_v0.2", "文档", "文档/创新创业大会/报送"]:
        st, body = api(f"{BASE}/repos/{REPO}/contents/{urllib.parse.quote(path)}")
        if st == 200:
            print(f"{path}: {len(body)} items")
        else:
            print(f"{path}: {st}")


if __name__ == "__main__":
    import urllib.parse
    main()