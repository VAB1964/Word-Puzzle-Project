import { defineConfig } from "vite";
import { fileURLToPath } from "node:url";

export default defineConfig({
  base: "./",
  envDir: "..",
  assetsInclude: ["**/*.csv", "**/*.wav", "**/*.mp3"],
  build: {
    rollupOptions: {
      input: {
        wordpuzzle: fileURLToPath(new URL("./wordpuzzle/index.html", import.meta.url))
      }
    }
  },
  server: {
    fs: {
      allow: [".."]
    },
    proxy: {
      "/api/wordpuzzle": {
        target: "http://127.0.0.1:8790",
        ws: true
      },
      "/wordpuzzle/room": {
        target: "http://127.0.0.1:8790"
      }
    }
  }
});
