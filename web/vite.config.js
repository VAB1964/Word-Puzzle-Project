import { defineConfig } from "vite";

export default defineConfig({
  base: "./",
  envDir: "..",
  assetsInclude: ["**/*.csv", "**/*.wav", "**/*.mp3"],
  server: {
    fs: {
      allow: [".."]
    }
  }
});
