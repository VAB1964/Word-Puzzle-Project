import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const webRoot = path.resolve(__dirname, "..");
const sourceDist = path.resolve(webRoot, "../cribbage-transfer/cribbage/dist");
const targetDir = path.resolve(webRoot, "dist/cribbage");

async function ensureSourceExists() {
  try {
    const stats = await fs.stat(sourceDist);
    if (!stats.isDirectory()) throw new Error(`${sourceDist} is not a directory`);
  } catch {
    throw new Error(`Cribbage build output not found at ${sourceDist}. Run cribbage build first.`);
  }
}

async function main() {
  await ensureSourceExists();
  await fs.rm(targetDir, { recursive: true, force: true });
  await fs.mkdir(targetDir, { recursive: true });
  await fs.cp(sourceDist, targetDir, { recursive: true });
  console.log(`Copied Cribbage dist contents to ${targetDir}`);
}

main().catch((error) => {
  console.error(error.message);
  process.exit(1);
});
