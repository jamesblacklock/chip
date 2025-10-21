@echo off

mkdir chip || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir chip || (echo Error occurred. Setup failed. && pause && exit /b 1)
mkdir deps || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir deps || (echo Error occurred. Setup failed. && pause && exit /b 1)
mkdir windows || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir windows || (echo Error occurred. Setup failed. && pause && exit /b 1)
mkdir bin || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir bin || (echo Error occurred. Setup failed. && pause && exit /b 1)
mkdir python || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir python || (echo Error occurred. Setup failed. && pause && exit /b 1)

curl https://www.python.org/ftp/python/3.13.9/python-3.13.9-embed-amd64.zip --output python-3.13.9-embed-amd64.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)
tar -xf python-3.13.9-embed-amd64.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)
del python-3.13.9-embed-amd64.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)

chdir .. || (echo Error occurred. Setup failed. && pause && exit /b 1)
mkdir git || (echo Error occurred. Setup failed. && pause && exit /b 1)
chdir git || (echo Error occurred. Setup failed. && pause && exit /b 1)

curl -L https://github.com/git-for-windows/git/releases/download/v2.51.1.windows.1/MinGit-2.51.1-busybox-64-bit.zip --output MinGit-2.51.1-busybox-64-bit.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)
tar -xf MinGit-2.51.1-busybox-64-bit.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)
del MinGit-2.51.1-busybox-64-bit.zip || (echo Error occurred. Setup failed. && pause && exit /b 1)

cd ../../../.. || (echo Error occurred. Setup failed. && pause && exit /b 1)

"./deps/windows/bin/git/cmd/git" init || (echo Error occurred. Setup failed. && pause && exit /b 1)
"./deps/windows/bin/git/cmd/git" remote add origin https://github.com/jamesblacklock/chip.git || (echo Error occurred. Setup failed. && pause && exit /b 1)
"./deps/windows/bin/git/cmd/git" pull origin main || (echo Error occurred. Setup failed. && pause && exit /b 1)

echo setup was successful
pause
