#!/bin/bash
# This file is maintained by velocitas CLI, do not modify manually. Change settings in .velocitas.json
set -e

# 定数
REPO_URL="https://gitlab.geniie.net/epfapi/dn_velocitas/quad-local.git"
CLONE_DIR="quad-local"  # vehicle-app-cpp-template直下に置く

# vehicle-app-cpp-template ディレクトリ直下で実行されている前提
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"  

cd "$WORKSPACE_DIR"

# クローン（すでにある場合はスキップ）
if [ -d "$CLONE_DIR" ]; then
    echo "[INFO] $CLONE_DIR は既に存在します。クローンをスキップします。"
else
    echo "[INFO] $REPO_URL をクローン中..."
    git clone "$REPO_URL" "$CLONE_DIR"
fi

# install.sh 実行
echo "[INFO] install.sh を実行します..."
cd "$CLONE_DIR/quad"
./install.sh

# # run.sh 実行（ログファイルへ出力してバックグラウンド起動）
# echo "[INFO] quad local を起動します..."
# ./run.sh > ../quad-local.log 2>&1
