import { getIssues } from "./data.js";
import { renderIssues } from "./issue.js";
import { createModal } from "./modal.js";
import { apiClient, fetchComments } from "./api.js";
import { tagStore, issueStore } from "./store.js";

const issuesListEl = document.getElementById("issues-list");
const unassignedBtn = document.getElementById("show-unassigned");
const createIssueBtn = document.getElementById("create-issue-btn");
const statusEl = document.getElementById("load-status");
const searchInput = document.getElementById("issue-search");
const statusButtons = document.querySelectorAll("[data-status]");

const normalizeRawId = (value) => {
  if (value === undefined || value === null) return null;
  const cleaned = value.toString().trim().replace(/^#/, "");
  if (!cleaned) return null;
  const asNum = Number(cleaned);
  return Number.isNaN(asNum) ? cleaned : asNum;
};

document.querySelectorAll(".nav-btn[data-target]").forEach((btn) => {
  btn.addEventListener("click", () => {
    const target = btn.dataset.target;
    if (target) {
      window.location.href = target;
    }
  });
});

const modal = createModal({
  onIssueUpdated: (updated) => {
    const hasRawId =
      updated.rawId !== undefined && updated.rawId !== null && updated.rawId !== "";
    const normalized = hasRawId ? { ...updated } : apiClient.mapIssue(updated);
    normalized.status = normalizeStatusValue(normalized.status || "");
    if (!normalized.database) {
      normalized.database = apiClient.getActiveDatabaseName();
    }
    const parsedRawId = normalizeRawId(normalized.rawId ?? normalized.id);
    if (parsedRawId !== null) {
      normalized.rawId = parsedRawId;
      if (!normalized.id || normalized.id === "#?") {
        normalized.id = `#${parsedRawId}`;
      }
    } else {
      // If we can't resolve an id, avoid polluting the list with a placeholder issue.
      console.warn("Skipping issue update with missing id", normalized);
      setStatus("Could not update issue: missing id from server.", true);
      return;
    }
    issueStore.upsertIssue({ ...normalized, rawId: normalized.rawId });
    renderFiltered();
  },
  getActiveDatabase: apiClient.getActiveDatabaseName
});

let activeDatabase = undefined;
let activeStatusFilter = "all";
let filterMode = "all"; // unassigned | assigned | all

const setStatus = (message, isError = false) => {
  if (!statusEl) return;
  statusEl.textContent = message;
  statusEl.classList.toggle("error", isError);
  statusEl.classList.add("show");
};

const normalizeStatusValue = (value) => {
  const normalized = (value || "").toString().trim().toLowerCase();
  if (normalized === "to be done" || normalized === "to do" || normalized === "todo") return "todo";
  if (normalized === "in progress" || normalized === "inprogress" || normalized === "progress") return "inprogress";
  if (normalized === "done") return "done";
  if (normalized === "backlog") return "backlog";
  return normalized;
};

const normalizeSearchId = (value) => {
  const trimmed = (value || "").trim();
  if (!trimmed) return null;
  const cleaned = trimmed.startsWith("#") ? trimmed.slice(1) : trimmed;
  const asNum = Number(cleaned);
  return Number.isNaN(asNum) ? null : asNum;
};

const matchesIssueId = (issue, id) => {
  if (issue.rawId === id) return true;
  const clean = `${issue.id || ""}`.replace(/^#/, "");
  return Number(clean) === id;
};

const getCachedIssues = () => issueStore.getState().issues || [];

const syncIssueTagsFromDefinitions = () => {
  const defs = tagStore.getState().definitions || [];
  const byId = new Map();
  const byName = new Map();
  defs.forEach((def) => {
    if (def.id !== null && def.id !== undefined) {
      byId.set(def.id, def);
    }
    if (def.label) {
      byName.set(def.label.toLowerCase(), def);
    }
  });

  let changed = false;
  const updatedIssues = getCachedIssues().map((issue) => {
    const tags = (issue.tags || []).map((t) => {
      const def =
        t.id !== undefined && t.id !== null
          ? byId.get(t.id)
          : byName.get((t.label || t.tag || "").toLowerCase());
      if (def) {
        if (t.label !== def.label || t.color !== def.color || t.id !== def.id) {
          changed = true;
        }
        return { ...t, id: def.id, label: def.label, color: def.color };
      }
      return t;
    }).filter((t) => t.id === undefined || t.id === null || byId.has(t.id));
    if (tags.length !== (issue.tags || []).length) {
      changed = true;
    }
    return { ...issue, tags };
  });
  if (changed) {
    issueStore.setIssues(updatedIssues);
  }
  if (changed) {
    renderFiltered();
  }
};

const filterIssuesByText = (issues = [], queryRaw = "") => {
  const query = (queryRaw || "").toLowerCase().trim();
  if (!query) return issues;
  const tokens = query.split(/\s+/).filter(Boolean);
  if (!tokens.length) return issues;

  return issues.filter((issue) => {
    const tagLabels = (issue.tags || []).map((t) => t.label || "").join(" ");
    const baseFields = [
      issue.id,
      issue.rawId,
      issue.author,
      issue.assignedTo,
      tagLabels
    ]
      .filter(Boolean)
      .map((f) => `${f}`.toLowerCase())
      .join(" ");

    return tokens.every((token) => {
      const [prefix, rest] = token.split(":", 2);
      if (rest !== undefined) {
        const value = rest.trim();
        if (!value) return true;
        if (["author", "by"].includes(prefix)) {
          return (issue.author || "").toLowerCase().includes(value);
        }
        if (["assignee", "assigned", "signee"].includes(prefix)) {
          return (issue.assignedTo || "").toLowerCase().includes(value);
        }
        return baseFields.includes(token);
      }
      return baseFields.includes(token);
    });
  });
};

const statusLabelByKey = (key) => {
  switch (key) {
    case "todo":
      return "To Be Done";
    case "inprogress":
      return "In Progress";
    case "done":
      return "Done";
    default:
      return "All";
  }
};

const renderAll = (issuesToRender = getCachedIssues()) => {
  renderIssues({
    container: issuesListEl,
    issues: issuesToRender,
    onOpen: (issue) => openIssueDetail(issue),
    onEdit: (issue) => modal.openEdit(issue),
    onDelete: (issue) => handleDelete(issue)
  });
};

const updateStatusButtons = () => {
  const counts = getCachedIssues().reduce(
    (acc, issue) => {
      const statusKey = normalizeStatusValue(issue.status);
      if (statusKey === "todo" || statusKey === "inprogress" || statusKey === "done") {
        acc[statusKey] += 1;
      }
      acc.all += 1;
      return acc;
    },
    { todo: 0, inprogress: 0, done: 0, all: 0 }
  );

  statusButtons.forEach((btn) => {
    const key = btn.dataset.status || "all";
    const label = statusLabelByKey(key);
    const count = counts[key] ?? counts.all;
    btn.textContent = `${label} (${count})`;
    btn.classList.toggle("is-active", key === activeStatusFilter);
    btn.setAttribute("aria-pressed", key === activeStatusFilter ? "true" : "false");
  });
};

const renderFiltered = (issuesToRender) => {
  let baseList = issuesToRender || getCachedIssues();
  if (!issuesToRender) {
    if (filterMode === "unassigned") {
      baseList = getCachedIssues().filter((i) => !i.assignedTo);
    } else if (filterMode === "assigned") {
      baseList = getCachedIssues().filter((i) => i.assignedTo);
    } else {
      baseList = getCachedIssues();
    }
  }

  const searchFiltered = filterIssuesByText(baseList, searchInput?.value || "");
  const statusFiltered =
    activeStatusFilter === "all"
      ? searchFiltered
      : searchFiltered.filter(
          (issue) => normalizeStatusValue(issue.status) === activeStatusFilter
        );

  renderAll(statusFiltered);
  updateStatusButtons();
};

const ensureIssueLoadedById = async (id) => {
  const current = getCachedIssues();
  const existingIdx = current.findIndex((issue) => matchesIssueId(issue, id));
  if (existingIdx >= 0) return current[existingIdx];

  const issue = await apiClient.fetchIssueById(id, activeDatabase);
  const updated = [...current];
  updated.unshift(issue);
  issueStore.setIssues(updated);
  return issue;
};

const handleSearchEnter = async () => {
  const parsedId = normalizeSearchId(searchInput?.value);
  if (parsedId !== null) {
    setStatus(`Searching for issue #${parsedId}...`);
    try {
      const issue = await ensureIssueLoadedById(parsedId);
      setStatus(`Found issue #${issue.rawId ?? parsedId}.`);
    } catch (err) {
      console.error("Failed to search issue by id:", err);
      const message = (err?.message || "").includes("404")
        ? `Issue #${parsedId} not found.`
        : "Search failed. Please try again.";
      setStatus(message, true);
    }
  }
  renderFiltered();
};

const handleSearchInput = () => {
  renderFiltered();
  if (searchInput?.value.trim()) {
    setStatus(`Filtered by search and status.`);
  }
};

const setAssignToggleLabel = () => {
  if (!unassignedBtn) return;
  if (filterMode === "unassigned") {
    unassignedBtn.textContent = "Unassigned only";
  } else if (filterMode === "assigned") {
    unassignedBtn.textContent = "Assigned only";
  } else {
    unassignedBtn.textContent = "All issues";
  }
};

const refreshFromApi = async () => {
  try {
    activeDatabase = await apiClient.fetchActiveDatabase();
  } catch (err) {
    console.error("Failed to load databases, continuing without:", err);
  }
  try {
    apiClient.setActiveDatabaseName(activeDatabase);
    const fetched = (await apiClient.fetchIssues()).map((issue) => ({
      ...issue,
      status: normalizeStatusValue(issue.status || "")
    }));
    issueStore.setIssues(fetched);
    const defsFromIssues = fetched.flatMap((i) => i.tags || []);
    tagStore.setDefinitions(defsFromIssues);
    syncIssueTagsFromDefinitions();
    setStatus(
      `Loaded ${fetched.length} issue(s) from API.${
        activeDatabase ? " Active DB: " + activeDatabase : ""
      }`
    );
  } catch (err) {
    console.error("Failed to load issues from API, using local data:", err);
    const fallback = getIssues();
    issueStore.setIssues(fallback);
    setStatus("API load failed; showing local mock data.", true);
  }
  renderFiltered();
};

const openIssueDetail = async (issue) => {
  try {
    const comments = await fetchComments(issue.rawId);
    modal.openDetail({ ...issue, comments });
  } catch (err) {
    console.error("Failed to load comments; showing issue without comments:", err);
    modal.openDetail(issue);
  }
};

createIssueBtn.addEventListener("click", () => {
  modal.openCreate();
});

searchInput?.addEventListener("keydown", (evt) => {
  if (evt.key === "Enter") {
    evt.preventDefault();
    handleSearchEnter();
  }
});

searchInput?.addEventListener("input", () => {
  handleSearchInput();
});

statusButtons.forEach((btn) => {
  btn.addEventListener("click", () => {
    const key = btn.dataset.status || "all";
    activeStatusFilter = key;
    renderFiltered();
  });
});

unassignedBtn?.addEventListener("click", () => {
  if (filterMode === "unassigned") {
    filterMode = "assigned";
    setAssignToggleLabel();
    renderFiltered();
    setStatus("Showing assigned issue(s).");
    return;
  }
  if (filterMode === "assigned") {
    filterMode = "all";
    setAssignToggleLabel();
    renderFiltered();
    setStatus("Showing all issue(s).");
    return;
  }
  filterMode = "unassigned";
  setAssignToggleLabel();
  renderFiltered();
  const count = getCachedIssues().filter((i) => !i.assignedTo).length;
  setStatus(`Showing ${count} unassigned issue(s).`);
});

const handleDelete = async (issue) => {
  if (!issue || issue.rawId === undefined || issue.rawId === null) {
    setStatus("Cannot delete: missing issue id.", true);
    return;
  }
  try {
    await apiClient.deleteIssue(issue.rawId);
    issueStore.removeIssue(issue.rawId);
    renderFiltered();
    setStatus(`Deleted issue #${issue.rawId}.`, false);
  } catch (err) {
    console.error("Failed to delete issue:", err);
    setStatus(err.message || "Failed to delete issue.", true);
  }
};

refreshFromApi();

tagStore.subscribe(() => {
  syncIssueTagsFromDefinitions();
});

issueStore.subscribe(() => {
  renderFiltered();
});
