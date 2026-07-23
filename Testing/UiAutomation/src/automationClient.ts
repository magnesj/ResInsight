import { APIRequestContext, expect } from "@playwright/test";

// Typed client for the ResInsight UI automation API. The shapes mirror
// docs/automation/openapi.yaml, which is the source of truth for this surface.

export interface Field {
  name: string;
  value: string;
  dataType?: string;
}

export interface PdmObject {
  address: string;
  keyword: string;
  uiName?: string;
  fields?: Field[];
  children?: PdmObject[];
}

export interface View {
  id: number;
  name: string;
  type?: string;
  address?: string;
}

export interface VisibleCellCount {
  viewId: number;
  visibleCellCount: number;
  totalCellCount?: number;
}

export interface CommandResponse {
  status: "ok" | "warning" | "error";
  messages?: string[];
}

export class AutomationClient {
  constructor(private readonly request: APIRequestContext) {}

  async health(): Promise<{ status: string; application: string; version: string }> {
    const response = await this.request.get("health");
    expect(response.ok()).toBeTruthy();
    return response.json();
  }

  async project(maxDepth?: number): Promise<PdmObject> {
    const response = await this.request.get("project", {
      params: maxDepth === undefined ? {} : { maxDepth },
    });
    expect(response.ok()).toBeTruthy();
    return response.json();
  }

  async object(address: string, maxDepth?: number): Promise<PdmObject> {
    const response = await this.request.get(`objects/${address}`, {
      params: maxDepth === undefined ? {} : { maxDepth },
    });
    expect(response.ok()).toBeTruthy();
    return response.json();
  }

  async setField(address: string, fieldName: string, value: string): Promise<PdmObject> {
    const response = await this.request.put(`objects/${address}/fields/${fieldName}`, {
      data: { value },
    });
    expect(response.ok(), await response.text()).toBeTruthy();
    return response.json();
  }

  async views(): Promise<View[]> {
    const response = await this.request.get("views");
    expect(response.ok()).toBeTruthy();
    return response.json();
  }

  async visibleCellCount(viewId: number): Promise<VisibleCellCount> {
    const response = await this.request.get(`views/${viewId}/visibleCellCount`);
    expect(response.ok()).toBeTruthy();
    return response.json();
  }

  async command(command: string): Promise<CommandResponse> {
    const response = await this.request.post("commands", { data: { command } });
    expect(response.ok(), await response.text()).toBeTruthy();
    return response.json();
  }

  // Depth-first search of the project tree for the first object with a given keyword.
  static findByKeyword(root: PdmObject, keyword: string): PdmObject | undefined {
    if (root.keyword === keyword) return root;
    for (const child of root.children ?? []) {
      const found = AutomationClient.findByKeyword(child, keyword);
      if (found) return found;
    }
    return undefined;
  }

  static findAllByKeyword(root: PdmObject, keyword: string): PdmObject[] {
    const matches: PdmObject[] = [];
    const visit = (object: PdmObject) => {
      if (object.keyword === keyword) matches.push(object);
      for (const child of object.children ?? []) visit(child);
    };
    visit(root);
    return matches;
  }
}
