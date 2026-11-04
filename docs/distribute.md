# How to distribute
The final user should run the following command:
> powershell -c "iex(irm 'https://is.gd/jetmyv2')"

The installer source is [`scripts/installer.ps1`](../scripts/installer.ps1).
When publishing this layout, update the short URL's destination to the hosted
`scripts/installer.ps1` path. Release executable names remain `JetmyV2.exe`.
