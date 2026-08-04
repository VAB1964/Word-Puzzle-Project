import { createSessionToken } from "./_tableTalkSession";

type Env = {
  TABLE_TALK_SESSION_SECRET?: string;
};

export const onRequestPost: PagesFunction<Env> = async (context) => {
  const secret = context.env.TABLE_TALK_SESSION_SECRET;
  if (!secret) {
    return new Response("Table Talk session secret not configured.", { status: 500 });
  }

  const { token, expiresAt } = await createSessionToken(secret);
  return Response.json({ token, expiresAt }, {
    headers: {
      "Cache-Control": "no-store",
    },
  });
};
