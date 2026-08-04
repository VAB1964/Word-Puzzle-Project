const SESSION_TTL_MS = 20 * 60 * 1000;

function toBase64Url(bytes: Uint8Array): string {
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return btoa(binary).replace(/\+/g, "-").replace(/\//g, "_").replace(/=+$/g, "");
}

function fromBase64Url(text: string): Uint8Array {
  const padded = text.replace(/-/g, "+").replace(/_/g, "/") + "=".repeat((4 - text.length % 4) % 4);
  const binary = atob(padded);
  const bytes = new Uint8Array(binary.length);
  for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);
  return bytes;
}

async function hmacSha256(payload: string, secret: string): Promise<string> {
  const keyData = new TextEncoder().encode(secret);
  const key = await crypto.subtle.importKey(
    "raw",
    keyData,
    { name: "HMAC", hash: "SHA-256" },
    false,
    ["sign"],
  );
  const signature = await crypto.subtle.sign("HMAC", key, new TextEncoder().encode(payload));
  return toBase64Url(new Uint8Array(signature));
}

export async function createSessionToken(secret: string): Promise<{ token: string; expiresAt: number }> {
  const expiresAt = Date.now() + SESSION_TTL_MS;
  const nonceBytes = new Uint8Array(12);
  crypto.getRandomValues(nonceBytes);
  const payload = JSON.stringify({
    exp: expiresAt,
    nonce: toBase64Url(nonceBytes),
  });
  const payloadB64 = toBase64Url(new TextEncoder().encode(payload));
  const sig = await hmacSha256(payloadB64, secret);
  return { token: `${payloadB64}.${sig}`, expiresAt };
}

export async function verifySessionToken(token: string, secret: string): Promise<boolean> {
  const [payloadB64, sig] = token.split(".");
  if (!payloadB64 || !sig) return false;
  const expectedSig = await hmacSha256(payloadB64, secret);
  if (sig !== expectedSig) return false;
  try {
    const payloadJson = new TextDecoder().decode(fromBase64Url(payloadB64));
    const payload = JSON.parse(payloadJson) as { exp?: number };
    if (!payload.exp || Date.now() > payload.exp) return false;
    return true;
  } catch {
    return false;
  }
}
