== Git Subtrees ==

* `src/7zip-cpp`: https://github.com/keithjjones/7zip-cpp.git master
* `src/7z`: https://github.com/pornel/7z.git original

They were added to the tree as follows:

```bash
URL=https://github.com/pornel/7z.git
PREFIX=src/7z
REF=original
REMOTE=pornel-7z
git remote add $REMOTE $URL
git remote update $REMOTE
git subtree add --prefix $PREFIX $REMOTE $REF --squash
```
