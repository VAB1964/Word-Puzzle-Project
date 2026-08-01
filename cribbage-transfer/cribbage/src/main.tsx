import React from "react";
import ReactDOM from "react-dom/client";
import CribbageGame from "./CribbageGame";
import "./styles.css";

ReactDOM.createRoot(document.getElementById("root")!).render(
  <React.StrictMode>
    <CribbageGame />
  </React.StrictMode>,
);
