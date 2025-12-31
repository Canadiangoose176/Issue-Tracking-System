const createStore = (initialState) => {
  let state = initialState;
  const listeners = new Set();

  const get = () => state;
  const set = (next) => {
    state = typeof next === "function" ? next(state) : next;
    listeners.forEach((fn) => {
      try {
        fn(state);
      } catch (err) {
        console.error("Store subscriber failed", err);
      }
    });
  };
  const subscribe = (fn) => {
    listeners.add(fn);
    return () => listeners.delete(fn);
  };

  return { get, set, subscribe };
};

const normalizeDefinition = (tag) => {
  const label = (tag && (tag.label || tag.tag)) || "";
  return {
    id: tag && tag.id !== undefined ? tag.id : null,
    label,
    color: (tag && tag.color) || "#49a3d8"
  };
};

// Normalize issue ids so comparisons work even when the backend sends strings.
const normalizeIssueId = (issue) => {
  const value = issue && (issue.rawId ?? issue.id);
  if (value === undefined || value === null) return null;
  const cleanedRaw = value.toString().trim();
  if (!cleanedRaw) return null;
  const cleaned = cleanedRaw.replace(/^#/, "");
  const asNum = Number(cleaned);
  if (!Number.isNaN(asNum)) return asNum;
  return cleaned;
};

const ensureIssueShape = (issue) => {
  if (!issue) return issue;
  const parsed = normalizeIssueId(issue);
  if (parsed === null) return { ...issue };
  return { ...issue, rawId: parsed, id: issue.id || `#${parsed}` };
};

const mergeIssueData = (existing, incoming) => {
  const base = ensureIssueShape(existing);
  const next = ensureIssueShape(incoming);
  if (!base) return next;
  const merged = { ...base, ...next };
  // Avoid overwriting a real title with the fallback placeholder.
  if (
    next &&
    next.title === "Untitled Issue" &&
    base.title &&
    base.title !== "Untitled Issue"
  ) {
    merged.title = base.title;
  }
  return merged;
};

const tagStore = (() => {
  const store = createStore({
    definitions: []
  });

  const mergeDefinitions = (incoming = []) => {
    const map = new Map();
    store.get().definitions.forEach((def) => {
      const key = def.id !== null && def.id !== undefined ? `id:${def.id}` : `name:${def.label.toLowerCase()}`;
      map.set(key, def);
    });
    incoming.forEach((t) => {
      const norm = normalizeDefinition(t);
      const key =
        norm.id !== null && norm.id !== undefined
          ? `id:${norm.id}`
          : `name:${norm.label.toLowerCase()}`;
      map.set(key, norm);
    });
    store.set({ definitions: Array.from(map.values()) });
  };

  const removeDefinition = (id) => {
    const next = store
      .get()
      .definitions.filter((def) => def.id !== id);
    store.set({ definitions: next });
  };

  return {
    subscribe: store.subscribe,
    getState: store.get,
    setDefinitions: mergeDefinitions,
    upsertDefinition: (tag) => mergeDefinitions([tag]),
    removeDefinition
  };
})();

const issueStore = (() => {
  const store = createStore({ issues: [] });

  const setIssues = (issues = []) => {
    const byId = new Map();
    const noId = [];

    issues.forEach((issue) => {
      const normalized = ensureIssueShape(issue);
      const key = normalizeIssueId(normalized);
      if (key === null) {
        // Drop truly unknown issues to avoid placeholder noise.
        if (normalized.rawId !== undefined || normalized.id !== undefined) {
          noId.push(normalized);
        }
        return;
      }
      const existing = byId.get(key);
      byId.set(key, mergeIssueData(existing, normalized));
    });

    store.set({ issues: [...byId.values(), ...noId] });
  };

  const upsertIssue = (issue) => {
    if (!issue) return;
    const current = store.get().issues || [];
    const incomingId = normalizeIssueId(issue);
    if (incomingId === null) {
      // Ignore updates without an id; they create placeholders in the UI.
      return;
    }
    const normalized = ensureIssueShape(issue);
    const idx = current.findIndex((i) => normalizeIssueId(i) === incomingId);
    if (idx >= 0) {
      const next = [...current];
      next[idx] = mergeIssueData(current[idx], normalized);
      store.set({ issues: next });
      return;
    }
    store.set({ issues: [normalized, ...current] });
  };
  const removeIssue = (rawId) => {
    const current = store.get().issues || [];
    const target = normalizeIssueId({ rawId });
    store.set({
      issues: current.filter((i) => normalizeIssueId(i) !== target)
    });
  };
  const mapIssues = (fn) => {
    const current = store.get().issues || [];
    store.set({ issues: current.map(fn) });
  };

  return {
    subscribe: store.subscribe,
    getState: store.get,
    setIssues,
    upsertIssue,
    removeIssue,
    mapIssues
  };
})();

export { tagStore, issueStore };
