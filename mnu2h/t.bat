@set PYTHONPYCACHEPREFIX=%~dp0..\tmp\pycache
@python -m coverage run -m unittest discover tests
@if %errorlevel% == 0 (
    @python -m coverage report
    @for %%a in ("report" "rep" "r") do @if "%1" == %%a goto :report
)
@del .coverage
@goto :exit

:report
@python -m coverage html
@move .coverage ..\tmp
@rd /S /Q ..\tmp\htmlcov 2>nul
@move /Y htmlcov ..\tmp\

:exit
