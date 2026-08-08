
const { app, BrowserWindow } = require('electron');
const { createServer } = require('vite');


app.whenReady().then(async () => {

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

    const WINDOW = new BrowserWindow({
        center: true,
        autoHideMenuBar: true,
        frame: true,
        width: 1080,
        height: 720,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false,
            backgroundThrottling: false
        }
    });

    WINDOW.loadURL('http://localhost:9268/apps/editor/');
    WINDOW.maximize();

});