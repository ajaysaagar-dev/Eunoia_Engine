import { defineConfig } from "vite";

export default defineConfig({
    build: {
        target: "esnext",
        chunkSizeWarningLimit: 1500,
        rollupOptions: {
            output: {
                manualChunks(id) {
                    if (id.includes("@babylonjs/core")) {
                        return "babylon-core";
                    }
                    if (id.includes("@babylonjs/materials")) {
                        return "babylon-materials";
                    }
                    if (id.includes("@babylonjs/loaders")) {
                        return "babylon-loaders";
                    }
                }
            }
        }
    }
});
