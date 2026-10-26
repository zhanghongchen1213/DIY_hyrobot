#!/usr/bin/env python3
"""Restore project files and fetch pinned third-party models into ignored source/."""
import argparse, hashlib, json, os, pathlib, shutil, subprocess, urllib.request

def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(4*1024*1024),b''):h.update(b)
    return h.hexdigest()

def matches(path, entry):
    variants=[entry]+entry.get('previous_versions',[])
    candidates=[v for v in variants if path.is_file() and path.stat().st_size==v['size']]
    if not candidates:return False
    actual=digest(path)
    return any(actual==v['sha256'] for v in candidates)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--group',nargs='+',choices=['project','asr','qwen','all'],default=['project'])
    parser.add_argument('--verify-only',action='store_true')
    parser.add_argument('--destination',type=pathlib.Path,help='Override source directory')
    args=parser.parse_args();root=pathlib.Path(__file__).resolve().parents[1]
    manifest=json.loads((root/'resources/source-manifest.json').read_text(encoding='utf8'))
    dest=(args.destination or root/'source').resolve()
    groups={'project','asr','qwen'} if 'all' in args.group else set(args.group)
    for entry in manifest['files']:
        if entry['group'] not in groups:continue
        target=dest/entry['path']
        if not target.resolve().is_relative_to(dest):raise ValueError('Resource path escapes destination')
        if target.exists():
            if matches(target,entry):
                print('Verified '+entry['path'],flush=True);continue
            raise RuntimeError('Existing local file differs; preserve or rename it before retrying: '+str(target))
        if args.verify_only:raise RuntimeError('Missing resource: '+str(target))
        target.parent.mkdir(parents=True,exist_ok=True);part=target.with_name(target.name+'.part')
        if entry.get('local'):
            shutil.copy2(root/entry['local'],part)
        else:
            print('Download '+entry['url'],flush=True)
            curl=shutil.which('curl.exe' if os.name=='nt' else 'curl')
            if curl:
                subprocess.run([curl,'--fail','--location','--retry','5','--connect-timeout','30','--continue-at','-',entry['url'],'--output',str(part)],check=True)
            else:
                with urllib.request.urlopen(entry['url'],timeout=60) as f,part.open('wb') as out:shutil.copyfileobj(f,out)
        if not matches(part,entry):
            bad=part.with_name(part.name+'.bad');part.replace(bad)
            raise RuntimeError('SHA-256 mismatch, retained at '+str(bad))
        part.replace(target);print('Restored '+entry['path'],flush=True)
    print('Selected resources ready. Versions and checksums: resources/source-manifest.json')

if __name__=='__main__':main()
