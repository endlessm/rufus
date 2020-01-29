## Making a release

* Create a signed tag of the form `VERSION_w.x.y.z`
* Build it in the Release/x86_32 configuration, and compress it with [upx](https://github.com/upx/upx)
* Sign it
* Rename the signed executable to `endless-installer-vw.x.y.z.exe` and copy it to a fresh directory
* Create a symlink named `endless-installer-eosw.x.exe` (or `endless-installer-master.exe` for the master branch) to arrange for it to go into ISOs for that branch
* If w.x is the current release branch, also create a symlink named `endless-installer.exe` to arrange for it to be served to website viewers
* Use `sync-endless-installer` from `eos-image-server` to publish it to S3
