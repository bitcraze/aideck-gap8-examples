#!/usr/bin/env bash
set -e

full_path=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

cd ${full_path}

pip install pillow scipy
python train_classifier.py
