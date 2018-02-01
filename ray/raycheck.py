#!/usr/bin/env python3

import re
import sys
import os
import shutil
import subprocess
import argparse
import colorama
from colorama import Fore, Style
from zipfile import ZipFile
import tarfile
from scipy.misc import imread
import numpy as np
import hashlib
from math import sqrt

def _msg(text, level, color):
    return Style.BRIGHT+color+level+Fore.RESET+Style.NORMAL+text

def compare(imagefn, reffn, rel, args):
    image = imread(imagefn)
    ref = imread(reffn)
    image = image.reshape((-1, 1))
    ref = ref.reshape((-1, 1))
    '''
    Root-mean-square error
    '''
    delta = image - ref
    rms = np.linalg.norm(delta) / sqrt(delta.size)
    if rms < args.maxrms:
        print(_msg(rel + ' RMS: {}'.format(rms), '[PASS] ', Fore.GREEN))
    else:
        print(_msg(rel + ' RMS: {}'.format(rms), '[WARNING] ', Fore.YELLOW))

def check_dirs(dirs):
    for d in dirs:
        if not os.path.exists(d):
            print('{} does not exist'.format(d))
            return False
        if not os.path.isdir(d):
            print('{} is not a directory'.format(d))
            return False
    return True

def check_files(files):
    for f in files:
        if not os.path.exists(f):
            print('{} does not exist'.format(f))
            return False
        if not os.path.isfile(f):
            print('{} is not a file'.format(f))
            return False
    return True

BUF_SIZE = 32 * 1024

def check_ref_signature(refcache_dir, refbin):
    sha = hashlib.sha256()
    signature_path = os.path.join(refcache_dir, 'signature')
    with open(refbin, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha.update(data)
    need_update = False
    if not os.path.exists(signature_path):
        signature = sha.hexdigest()
        need_update = True
    else:
        with open(signature_path, 'rb') as f:
            signature = f.readline()
        if signature != sha.hexdigest():
            signature = sha.hexdigest()
            need_update = True
    if need_update:
        with open(signature_path, 'w') as f:
            f.write(signature)
    return need_update

def raycheck(args):
    scene_dir = args.scenes
    os.makedirs(args.out, exist_ok=True)
    if not check_dirs([scene_dir, args.out]) or not check_files([args.exec, args.ref]):
        return
    refcache_dir = os.path.join(args.out, 'refcache')
    os.makedirs(os.path.join(args.out, 'refcache'), exist_ok=True)
    if not check_ref_signature(refcache_dir, args.ref):
        shutil.rmtree(refcache_dir)
    for root, dirs, files in os.walk(scene_dir):
        rel_dir = os.path.relpath(root, start=scene_dir)
        # print('root {} rel {}'.format(root, rel_dir))
        os.makedirs(os.path.join(args.out, 'image', rel_dir), exist_ok=True)
        os.makedirs(os.path.join(args.out, 'refcache', rel_dir), exist_ok=True)
        os.makedirs(os.path.join(args.out, 'stdio', rel_dir), exist_ok=True)
        for fn in files:
            if not fn.endswith('.ray'):
                continue
            rayfn = os.path.join(root, fn)
            ray_rel_from_scene = os.path.relpath(rayfn, start=scene_dir)
            relbase, _ = os.path.splitext(ray_rel_from_scene)
            # print('processing {}'.format(relbase))
            imagefn = os.path.join(args.out, 'image', relbase + '.png')
            refimagefn = os.path.join(args.out, 'refcache', relbase + '.std.png')
            stdoutfn = os.path.join(args.out, 'stdio', relbase + '.out')
            stderrfn = os.path.join(args.out, 'stdio', relbase + '.err')
            # print('{} -> {} vs {}'.format(rayfn, imagefn, refimagefn))
            if not os.path.exists(refimagefn):
                subprocess.run([args.ref, '-r', '5', rayfn, refimagefn])
            with open(stdoutfn, 'w') as stdout, open(stderrfn, 'w') as stderr:
                subprocess.run([args.exec, '-r', '5', rayfn, imagefn], stdout=stdout, stderr=stderr, timeout=args.timelimit)
            compare(imagefn, refimagefn, relbase, args)

if __name__ == '__main__':
    colorama.init()
    parser = argparse.ArgumentParser(description='Grading your ray tracer',
            formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--exec', metavar='RAY',
            help='Executable file of your ray tracer',
            default='build/bin/ray')
    parser.add_argument('--ref', metavar='RAY.STD',
            help='Reference ray tracer',
            default='ray-solution')
    parser.add_argument('--scenes', metavar='DIRECTORY',
            help='Directory that stores .ray files.',
            default='assets/scenes')
    parser.add_argument('--out', metavar='DIRECTORY',
            help='Output directory',
            default='raycheck.out')
    parser.add_argument('--timelimit', metavar='SECONDS',
            help='Time limit in seconds, NOTE: the default limit is used during grading',
            type=int,
            default=180)
    parser.add_argument('--maxrms', metavar='NUMBER',
            help='Maximum allowed root-mean-square error',
            type=float,
            default=10.0)
    args = parser.parse_args()
    raycheck(args)
