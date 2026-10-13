#!/usr/bin/env python3
"""Merge a committed local component branch into DIY_hyrobot using squash subtree."""
import argparse, json, os, pathlib, subprocess

def run(repo,*args,capture=False):
    command=['git','-C',str(repo),*map(str,args)]
    if capture:return subprocess.check_output(command,encoding='utf8').strip()
    subprocess.run(command,check=True)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--component',required=True,choices=['hy_ros','hy_driver','butter_yolov8n_train'])
    parser.add_argument('--source',type=pathlib.Path,required=True,help='Existing independent Git checkout')
    parser.add_argument('--branch',help='Defaults to the source checkout current branch')
    parser.add_argument('--push',action='store_true',help='Fast-forward from remote main first, then publish')
    args=parser.parse_args()
    root=pathlib.Path(__file__).resolve().parents[1];source=args.source.resolve()
    if pathlib.Path(run(source,'rev-parse','--show-toplevel',capture=True)).resolve()!=source:
        parser.error('--source must be the root of the independent repository')
    if source==root:parser.error('The source must be an independent component repository')
    branch=args.branch or run(source,'branch','--show-current',capture=True)
    if not branch:parser.error('Specify --branch for a detached source checkout')
    run(source,'check-ref-format','--branch',branch,capture=True)
    source_sha=run(source,'rev-parse','--verify','refs/heads/'+branch,capture=True)
    for path in [root,source]:
        if run(path,'status','--porcelain',capture=True):parser.error('Commit or stash changes first: '+str(path))
    if run(root,'branch','--show-current',capture=True)!='main':parser.error('Check out main in the monorepo first')
    git_dir=pathlib.Path(run(root,'rev-parse','--absolute-git-dir',capture=True));lock=git_dir/'component-sync.lock'
    try:fd=os.open(lock,os.O_CREAT|os.O_EXCL|os.O_WRONLY,0o600)
    except FileExistsError:parser.error('A component sync is already running; inspect '+str(lock))
    os.close(fd)
    try:
        if args.push:
            run(root,'fetch','origin','main')
            run(root,'merge','--ff-only','FETCH_HEAD')
        before=run(root,'rev-parse','HEAD',capture=True)
        # --squash deliberately keeps the independent repository's old secrets and
        # obsolete binary history out of the cloud monorepo's reachable history.
        run(root,'subtree','pull','--prefix='+args.component,str(source),branch,'--squash',
            '-m',f'sync({args.component}): import {branch} at {source_sha}')
        after=run(root,'rev-parse','HEAD',capture=True)
        changed=run(root,'diff','--name-only',before,after,capture=True).splitlines()
        if any(not p.startswith(args.component+'/') for p in changed):raise RuntimeError('Unexpected changes outside component; inspect before pushing')
        if args.push:run(root,'push','origin','HEAD:refs/heads/main')
        print(json.dumps({'component':args.component,'source_branch':branch,'source_commit':source_sha,
            'monorepo_commit':after,'changed_files':len(changed),'pushed':args.push},indent=2))
    finally:lock.unlink()

if __name__=='__main__':main()
