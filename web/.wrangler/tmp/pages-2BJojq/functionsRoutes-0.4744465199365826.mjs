import { onRequestPost as __api_table_talk_generate_ts_onRequestPost } from "E:\\UdemyCoursesProjects\\WordPuzzle\\SFML_TestProject\\web\\functions\\api\\table-talk-generate.ts"
import { onRequestPost as __api_table_talk_session_ts_onRequestPost } from "E:\\UdemyCoursesProjects\\WordPuzzle\\SFML_TestProject\\web\\functions\\api\\table-talk-session.ts"
import { onRequestPost as __api_table_talk_tts_ts_onRequestPost } from "E:\\UdemyCoursesProjects\\WordPuzzle\\SFML_TestProject\\web\\functions\\api\\table-talk-tts.ts"

export const routes = [
    {
      routePath: "/api/table-talk-generate",
      mountPath: "/api",
      method: "POST",
      middlewares: [],
      modules: [__api_table_talk_generate_ts_onRequestPost],
    },
  {
      routePath: "/api/table-talk-session",
      mountPath: "/api",
      method: "POST",
      middlewares: [],
      modules: [__api_table_talk_session_ts_onRequestPost],
    },
  {
      routePath: "/api/table-talk-tts",
      mountPath: "/api",
      method: "POST",
      middlewares: [],
      modules: [__api_table_talk_tts_ts_onRequestPost],
    },
  ]