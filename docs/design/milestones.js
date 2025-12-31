import {
  fetchMilestones,
  createMilestone,
  updateMilestone,
  deleteMilestone,
  addIssueToMilestone,
  removeIssueFromMilestone,
  fetchMilestone,
  fetchMilestoneIssues,
  fetchIssues
} from "./api.js";
import { renderMilestones } from "./milestone.js";

const listEl = document.getElementById("milestone-list");
const emptyEl = document.getElementById("milestone-empty");
const statusEl = document.getElementById("milestone-status");
const searchInput = document.getElementById("milestone-search");
const createBtn = document.getElementById("create-milestone-btn");

document.querySelectorAll(".nav-btn[data-target]").forEach((btn) => {
  btn.addEventListener("click", () => {
    const target = btn.dataset.target;
    if (target) {
      window.location.href = target;
    }
  });
});

let milestones = [];
let draftState = {
  open: false,
  mode: "create",
  editingId: null,
  initial: {
    name: "",
    description: "",
    startDate: "",
    endDate: ""
  }
};
let expandedIds = new Set();
let issueSuggestions = [];
const issuesByMilestone = new Map();

const setStatus = (msg, isError = false) => {
  if (!statusEl) return;
  statusEl.textContent = msg || "";
  statusEl.classList.toggle("error", Boolean(isError));
};

const filterMilestones = () => {
  const term = (searchInput?.value || "").trim().toLowerCase();
  if (!term) return milestones;
  return milestones.filter((m) => {
    const haystack = `${m.name} ${m.description} ${m.id}`.toLowerCase();
    return haystack.includes(term);
  });
};

const closeDraft = () => {
  draftState = {
    open: false,
    mode: "create",
    editingId: null,
    initial: { name: "", description: "", startDate: "", endDate: "" }
  };
};

const openDraft = (milestone = null) => {
  draftState = {
    open: true,
    mode: milestone ? "edit" : "create",
    editingId: milestone?.id || null,
    initial: {
      name: milestone?.name || "",
      description: milestone?.description || "",
      startDate: milestone?.startDate || "",
      endDate: milestone?.endDate || ""
    }
  };
  renderList();
};

const buildDraftRow = () => {
  const form = document.createElement("form");
  form.className = "row draft";

  const fields = document.createElement("div");
  fields.className = "draft-fields";

  const nameInput = document.createElement("input");
  nameInput.type = "text";
  nameInput.placeholder = "Milestone name";
  nameInput.className = "input-title";
  nameInput.required = true;
  nameInput.value = draftState.initial.name;

  const descInput = document.createElement("input");
  descInput.type = "text";
  descInput.placeholder = "Description";
  descInput.className = "input-desc";
  descInput.value = draftState.initial.description;

  const startInput = document.createElement("input");
  startInput.type = "date";
  startInput.required = true;
  startInput.value = draftState.initial.startDate;

  const endInput = document.createElement("input");
  endInput.type = "date";
  endInput.required = true;
  endInput.value = draftState.initial.endDate;

  fields.append(nameInput, descInput, startInput, endInput);

  const save = document.createElement("button");
  save.type = "submit";
  save.className = "btn";
  save.textContent = draftState.mode === "edit" ? "Save" : "Create";

  const cancel = document.createElement("button");
  cancel.type = "button";
  cancel.className = "btn cancel";
  cancel.textContent = "Cancel";
  cancel.addEventListener("click", () => {
    closeDraft();
    renderList();
  });

  form.append(fields, save, cancel);

  form.addEventListener("submit", async (evt) => {
    evt.preventDefault();
    const payload = {
      name: nameInput.value.trim(),
      description: descInput.value.trim(),
      startDate: startInput.value,
      endDate: endInput.value
    };

    if (!payload.name || !payload.startDate || !payload.endDate) {
      setStatus("Name, start date, and end date are required.", true);
      return;
    }

    try {
      if (draftState.mode === "edit" && draftState.editingId !== null) {
        setStatus("Updating milestone...");
        const updated = await updateMilestone(draftState.editingId, payload);
        milestones = milestones.map((m) =>
          m.id === draftState.editingId ? updated : m
        );
        setStatus(`Updated ${updated.name}.`);
      } else {
        setStatus("Creating milestone...");
        const created = await createMilestone(payload);
        milestones = [created, ...milestones];
        setStatus(`Created ${created.name}.`);
      }
      closeDraft();
      renderList();
    } catch (err) {
      console.error("Milestone save failed", err);
      setStatus(err.message || "Failed to save milestone.", true);
    }
  });

  setTimeout(() => nameInput.focus(), 0);
  return form;
};

const handleDelete = async (milestone) => {
  try {
    setStatus("Deleting milestone...");
    await deleteMilestone(milestone.id);
    milestones = milestones.filter((m) => m.id !== milestone.id);
    expandedIds.delete(milestone.id);
    issuesByMilestone.delete(milestone.id);
    renderList();
    setStatus(`Deleted ${milestone.name}.`);
  } catch (err) {
    console.error("Failed to delete milestone", err);
    setStatus(err.message || "Failed to delete milestone.", true);
  }
};

const parseIssueId = (raw) => {
  const text = (raw || "").toString().trim();
  if (!text) return null;
  const match = text.match(/#?(\d+)/);
  if (match) {
    const num = Number(match[1]);
    return Number.isNaN(num) ? match[1] : num;
  }
  return text;
};

const ensureIssuesLoaded = async (milestoneId) => {
  if (issuesByMilestone.has(milestoneId)) return;
  const list = await fetchMilestoneIssues(milestoneId);
  issuesByMilestone.set(milestoneId, list);
};

const handleAddIssue = async (milestone, rawValue) => {
  const issueId = parseIssueId(rawValue);
  if (issueId === null || issueId === undefined || issueId === "") {
    setStatus("Issue ID is required.", true);
    return false;
  }

  try {
    setStatus("Linking issue...");
    await addIssueToMilestone(milestone.id, issueId);
    const [refreshed, issues] = await Promise.all([
      fetchMilestone(milestone.id),
      fetchMilestoneIssues(milestone.id)
    ]);
    milestones = milestones.map((m) => (m.id === milestone.id ? refreshed : m));
    issuesByMilestone.set(milestone.id, issues);
    renderList();
    setStatus(`Added issue #${issueId} to ${milestone.name}.`);
    return true;
  } catch (err) {
    console.error("Failed to add issue to milestone", err);
    setStatus(err.message || "Failed to add issue.", true);
    return false;
  }
};

const handleRemoveIssue = async (milestone, issueId) => {
  try {
    setStatus("Removing issue...");
    await removeIssueFromMilestone(milestone.id, issueId);
    const [refreshed, issues] = await Promise.all([
      fetchMilestone(milestone.id),
      fetchMilestoneIssues(milestone.id)
    ]);
    milestones = milestones.map((m) => (m.id === milestone.id ? refreshed : m));
    issuesByMilestone.set(milestone.id, issues);
    renderList();
    setStatus(`Removed issue #${issueId} from ${milestone.name}.`);
  } catch (err) {
    console.error("Failed to remove issue from milestone", err);
    setStatus(err.message || "Failed to remove issue.", true);
  }
};

const renderList = () => {
  if (!listEl) return;
  listEl.innerHTML = "";

  const filtered = filterMilestones();
  const fragment = document.createDocumentFragment();

  if (draftState.open) {
    fragment.appendChild(buildDraftRow());
  }

  if (!filtered.length && !draftState.open) {
    emptyEl?.classList.remove("hidden");
  } else {
    emptyEl?.classList.add("hidden");
    const cardsContainer = document.createElement("div");
    renderMilestones({
      container: cardsContainer,
      milestones: filtered,
      onOpen: (m) =>
        setStatus(
          `${m.name}: ${m.startDate || "?"} → ${m.endDate || "?"} (${m.description || "No description"})`
        ),
      onEdit: (m) => openDraft(m),
      onDelete: (m) => handleDelete(m),
      onAddIssue: (m, value) => handleAddIssue(m, value),
      onToggle: async (m, expanded) => {
        if (expanded) {
          expandedIds.add(m.id);
          try {
            await ensureIssuesLoaded(m.id);
          } catch (err) {
            console.error("Failed to load milestone issues", err);
            setStatus(err.message || "Failed to load issues for milestone.", true);
          }
        } else {
          expandedIds.delete(m.id);
        }
      },
      issueOptions: issueSuggestions,
      issuesMap: issuesByMilestone,
      onRemoveIssue: (m, issue) => handleRemoveIssue(m, issue.rawId ?? issue.id),
      expandedIds
    });
    fragment.append(...Array.from(cardsContainer.childNodes));
  }

  listEl.replaceChildren(fragment);
};

const load = async () => {
  try {
    setStatus("Loading milestones...");
    const [milestoneList, issueList] = await Promise.all([
      fetchMilestones(),
      fetchIssues()
    ]);
    milestones = milestoneList;
    issueSuggestions = issueList;
    setStatus(
      `Loaded ${milestones.length} milestone${milestones.length === 1 ? "" : "s"}.`
    );
    renderList();
  } catch (err) {
    console.error("Failed to load milestones", err);
    setStatus(err.message || "Failed to load milestones.", true);
    renderList();
  }
};

createBtn?.addEventListener("click", () => openDraft());
searchInput?.addEventListener("input", renderList);

load();
