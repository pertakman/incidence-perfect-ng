# Beta Version And Tag Commands

Use this sequence to produce a traceable beta candidate.

## 1) Clean Working Tree

```powershell
git status
```

## 2) Choose Beta Version

Example:

```powershell
$env:FW_VERSION_OVERRIDE="2026.2.6-beta1"
```

## 3) Build And Upload

```powershell
pio run -e esp32s3
pio run -e esp32s3 -t upload
```

## 4) Verify Runtime Version

- Confirm splash overlay shows `2026.2.6-beta1`.
- Confirm serial startup banner reports same FW version.

## 5) Commit (If Needed) And Tag

```powershell
git add .
git commit -m "Prepare beta 2026.2.6-beta1"
git tag -a v2026.2.6-beta1 -m "Beta build 2026.2.6-beta1"
```

## 6) Push

```powershell
git push
git push --tags
```

## 7) Clear Override For Normal Builds

```powershell
Remove-Item Env:FW_VERSION_OVERRIDE
```
