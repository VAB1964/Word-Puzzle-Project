import { defineConfig } from "vite";

export default defineConfig({
  base: "./",
  assetsInclude: ["**/*.csv", "**/*.wav", "**/*.mp3"],
  server: {
    fs: {
      allow: [".."]
    }
  }
});
