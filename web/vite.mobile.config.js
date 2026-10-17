import { defineConfig } from "vite";
import { fileURLToPath } from "node:url";

export default defineConfig({
  root: fileURLToPath(new URL("./mobile", import.meta.url)),
  base: "./",
  envDir: ".",
  publicDir: false,
  assetsInclude: ["**/*.csv", "**/*.wav", "**/*.mp3"],
  server: {
    fs: {
      allow: ["../.."]
    }
  },
  build: {
    outDir: fileURLToPath(new URL("./dist-mobile", import.meta.url)),
    emptyOutDir: true
  }
});
