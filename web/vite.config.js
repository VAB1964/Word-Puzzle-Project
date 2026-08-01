import { defineConfig } from "vite";
import { fileURLToPath } from "node:url";

export default defineConfig({
  base: "./",
  envDir: "..",
  assetsInclude: ["**/*.csv", "**/*.wav", "**/*.mp3"],
  build: {
    rollupOptions: {
      input: {
        main: fileURLToPath(new URL("./index.html", import.meta.url)),
        wordpuzzle: fileURLToPath(new URL("./wordpuzzle/index.html", import.meta.url))
      }
    }
  },
  server: {
    fs: {
      allow: [".."]
    }
  }
});
