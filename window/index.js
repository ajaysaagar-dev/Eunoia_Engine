const { app, BrowserWindow } = require('electron');
const path = require('path');
const fs = require('fs');

// Hardware acceleration flags for maximum rendering performance
app.commandLine.appendSwitch('enable-gpu-rasterization');
app.commandLine.appendSwitch('enable-zero-copy');
app.commandLine.appendSwitch('ignore-gpu-blacklist');

app.whenReady().then(async () => {
    const prodDistPath = path.join(__dirname, '../apps/editor/dist/index.html');
    const isProd = fs.existsSync(prodDistPath);

    let loadTarget = '';

    if (isProd) {
        loadTarget = prodDistPath;
    } else {
        const { createServer } = require('vite');
        const SERVER = await createServer({
            root: '.',
            server: {
                watch: true,
                fs: {
                    allow: ['.'],
                    strict: false
                }
            }
        });
        await SERVER.listen(9268);
        loadTarget = 'http://localhost:9268/apps/editor/';
    }

    const WINDOW = new BrowserWindow({
        center: true,
        autoHideMenuBar: true,
        frame: true,
        width: 1080,
        height: 720,
        show: false,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            backgroundThrottling: false
        }
    });

    WINDOW.once('ready-to-show', () => {
        WINDOW.maximize();
        WINDOW.show();
    });

    if (isProd) {
        WINDOW.loadFile(loadTarget);
    } else {
        WINDOW.loadURL(loadTarget);
    }
});