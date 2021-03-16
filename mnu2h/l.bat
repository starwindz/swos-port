@set PYTHONPYCACHEPREFIX=%~dp0..\tmp\pycache
@for /r %%v in (*.py) do pylint -d no-else-return "%%v"
