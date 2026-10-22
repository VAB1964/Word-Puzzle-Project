import "../style.css";
import "./style.css";
import { LachlanGame } from "./game";

const root = document.getElementById("lachlanApp");
if (!root) throw new Error("Missing #lachlanApp.");

new LachlanGame(root);
