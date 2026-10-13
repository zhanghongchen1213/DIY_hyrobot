#!/usr/bin/env python3
"""Download and verify DIY_hyrobot resources using the committed manifest."""
import argparse, hashlib, json, os, pathlib, shutil, subprocess, tempfile, urllib.request

def sha256(path):
    with path.open('rb') as f:
        h=hashlib.sha256()
        for chunk in iter(lambda:f.read(4*1024*1024),b''): h.update(chunk)
        return h.hexdigest()

def safe_path(root, relative):
    rel=pathlib.PurePosixPath(relative)
    if rel.is_absolute() or '..' in rel.parts or '\\' in relative or ':' in relative:
        raise ValueError('Unsafe resource path: '+relative)
    target=root.joinpath(*rel.parts)
    if not target.resolve().is_relative_to(root.resolve()): raise ValueError('Path escapes destination')
    return target

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--group',nargs='+',default=['Solidworks'],help='Solidworks (hardware only), or all')
    parser.add_argument('--root',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1])
    parser.add_argument('--cache',type=pathlib.Path)
    parser.add_argument('--sevenzip',help='Path to 7z / 7zz / 7za / 7zr')
    parser.add_argument('--download-only',action='store_true')
    parser.add_argument('--verify-only',action='store_true',help='Verify extracted files without downloading')
    args=parser.parse_args()
    root=args.root.resolve()
    manifest=json.loads((pathlib.Path(__file__).resolve().parents[1]/'resources/resources-manifest.json').read_text(encoding='utf8'))
    known={p['group'] for p in manifest['packages']}
    selected=known if args.group==['all'] else set(args.group)
    if selected-known: parser.error('Unknown groups: '+', '.join(sorted(selected-known)))
    cache=(args.cache or root/'.resource-cache').resolve();cache.mkdir(parents=True,exist_ok=True)
    exe=args.sevenzip or next((shutil.which(x) for x in ['7zz','7z','7za','7zr'] if shutil.which(x)),None)
    if not exe and os.name=='nt':
        p=pathlib.Path(os.environ.get('ProgramFiles','C:/Program Files'))/'7-Zip/7z.exe'
        if p.exists():exe=str(p)
    if not args.download_only and not args.verify_only and not exe:parser.error('Install 7-Zip (Ubuntu: sudo apt install p7zip-full), or supply --sevenzip')
    for package in manifest['packages']:
        if package['group'] not in selected:continue
        dest=safe_path(root,package['destination'])
        files=package['files']
        if args.verify_only:
            for entry in files:
                f=safe_path(dest,entry['path'])
                if not f.is_file() or f.stat().st_size!=entry['size'] or sha256(f)!=entry['sha256']:
                    raise RuntimeError('Missing or changed resource: '+str(f))
            print(package['group']+': verified '+str(len(files))+' files',flush=True);continue
        for asset in package['assets']:
            target=safe_path(cache,asset['name'])
            if not target.exists() or target.stat().st_size!=asset['size'] or sha256(target)!=asset['sha256']:
                url=f'https://github.com/{manifest["repository"]}/releases/download/{manifest["release"]}/{asset["name"]}'
                part=target.with_name(target.name+'.part')
                print('Downloading '+asset['name'],flush=True)
                curl=shutil.which('curl.exe' if os.name=='nt' else 'curl')
                if curl:
                    subprocess.run([curl,'--fail','--location','--retry','5','--connect-timeout','30','--continue-at','-',url,'--output',str(part)],check=True)
                else:
                    with urllib.request.urlopen(url,timeout=60) as response,part.open('wb') as out:shutil.copyfileobj(response,out)
                if part.stat().st_size!=asset['size'] or sha256(part)!=asset['sha256']:
                    # A hash mismatch is not resumable; retain the bytes for inspection.
                    bad=part.with_name(part.name+'.bad');part.replace(bad)
                    raise RuntimeError('Checksum mismatch: '+asset['name']+'; retained as '+str(bad))
                part.replace(target)
            print('Verified '+asset['name'],flush=True)
        if args.download_only:continue
        # Extract into a temporary location; never overwrite a different local resource.
        with tempfile.TemporaryDirectory(prefix='extract-',dir=cache) as tmp:
            stage=pathlib.Path(tmp)
            subprocess.run([exe,'x',str(cache/package['first_volume']),'-o'+str(stage),'-y','-bsp0'],check=True)
            expected={entry['path'] for entry in files}
            actual={f.relative_to(stage).as_posix() for f in stage.rglob('*') if f.is_file()}
            if actual!=expected:raise RuntimeError('Archive file list differs from manifest')
            for entry in files:
                f=safe_path(stage,entry['path']);out=safe_path(dest,entry['path'])
                if f.is_symlink() or f.stat().st_size!=entry['size'] or sha256(f)!=entry['sha256']:
                    raise RuntimeError('Extracted file mismatch: '+entry['path'])
                if out.exists() and (not out.is_file() or sha256(out)!=entry['sha256']):
                    raise RuntimeError('Local file differs; preserve or rename it before retrying: '+str(out))
            for entry in files:
                out=safe_path(dest,entry['path']);out.parent.mkdir(parents=True,exist_ok=True)
                if not out.exists():shutil.copy2(safe_path(stage,entry['path']),out)
        print(package['group']+': restored '+str(len(files))+' files to '+str(dest),flush=True)

if __name__=='__main__':main()
