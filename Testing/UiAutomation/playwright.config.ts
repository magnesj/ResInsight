import { defineConfig } from "@playwright/test";

// Base URL of a running ResInsight automation server. Start ResInsight with
//   ResInsight --automationserver 8080
// and (optionally) override the URL via the RESINSIGHT_AUTOMATION_URL env var.
const configuredURL =
  process.env.RESINSIGHT_AUTOMATION_URL ?? "http://127.0.0.1:8080/api/v1";

// Request paths are resolved against baseURL using WHATWG URL semantics, where the
// last path segment is replaced. The trailing slash keeps the /api/v1 prefix intact.
const baseURL = configuredURL.endsWith("/") ? configuredURL : `${configuredURL}/`;

export default defineConfig({
  testDir: "./tests",
  fullyParallel: false,
  workers: 1,
  reporter: process.env.CI ? "github" : "list",
  use: {
    baseURL,
    extraHTTPHeaders: {
      Accept: "application/json",
    },
  },
});
