import { defineConfig } from "@playwright/test";

// Base URL of a running ResInsight automation server. Start ResInsight with
//   ResInsight --automationserver 8080
// and (optionally) override the URL via the RESINSIGHT_AUTOMATION_URL env var.
const baseURL =
  process.env.RESINSIGHT_AUTOMATION_URL ?? "http://127.0.0.1:8080/api/v1";

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
