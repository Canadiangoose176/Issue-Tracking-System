import {
  fetchDatabases,
  createDatabase,
  deleteDatabase,
  switchDatabase,
  renameDatabase
} from "./api.js";

const createBtn = document.getElementById("create-db-btn");
const searchInput = document.getElementById("db-search");
const listEl = document.getElementById("db-list");
const emptyEl = document.getElementById("db-empty");
const statusEl = document.getElementById("db-status");
let currentDraftForm = null;

document.querySelectorAll(".nav-btn[data-target]").forEach((btn) => {
  btn.addEventListener("click", () => {
    const target = btn.dataset.target;
    if (target) window.location.href = target;
  });
});

let databases = [];
const stripDbExt = (name = "") =>
  name.toLowerCase().endsWith(".db") ? name.slice(0, -3) : name;
const withDbExt = (name = "") =>
  name.toLowerCase().endsWith(".db") ? name : `${name}.db`;

const setStatus = (msg, isError = false) => {
  if (!statusEl) return;
  statusEl.textContent = msg || "";
  statusEl.classList.toggle("error", Boolean(isError));
};

const sortDatabases = (list = []) =>
  [...list].sort((a, b) => {
    if (a.active && !b.active) return -1;
    if (!a.active && b.active) return 1;
    return a.name.localeCompare(b.name);
  });

let draftState = {
  open: false,
  mode: "create",
  editingName: null,
  initialName: ""
};

const closeDraft = () => {
  draftState = { open: false, mode: "create", editingName: null, initialName: "" };
  currentDraftForm = null;
};

const openDraft = ({ mode, name }) => {
  draftState = {
    open: true,
    mode,
    editingName: name || null,
    initialName: stripDbExt(name || "")
  };
  render();
};

const buildDraftRow = ({ initialName, onSubmit, onCancel }) => {
  const form = document.createElement("form");
  form.className = "db-row";
  currentDraftForm = form;

  const fields = document.createElement("div");
  fields.className = "db-draft-fields";

  const input = document.createElement("input");
  input.type = "text";
  input.placeholder = "New Database";
  input.className = "db-name-input";
  input.required = true;
  input.value = initialName || "";
  fields.appendChild(input);

  const cancel = document.createElement("button");
  cancel.type = "button";
  cancel.className = "btn";
  cancel.textContent = "Cancel";
  cancel.addEventListener("click", onCancel);

  form.append(fields, cancel);

  form.addEventListener("submit", async (evt) => {
    evt.preventDefault();
    const name = input.value.trim();
    if (!name) {
      input.focus();
      return;
    }
    try {
      await onSubmit({ name });
    } catch (err) {
      console.error("Database submit failed", err);
      setStatus(err.message || "Failed to save database.", true);
    }
  });

  setTimeout(() => input.focus(), 0);
  return form;
};

const buildRow = (db) => {
  const row = document.createElement("div");
  row.className = `db-row${db.active ? " active" : ""}`;

  const meta = document.createElement("div");
  meta.className = "db-meta";
  const strong = document.createElement("strong");
  strong.textContent = db.name;
  const status = document.createElement("span");
  status.className = "status";
  status.textContent = db.active ? "Active" : "Down";
  meta.append(strong, status);

  const actions = document.createElement("div");
  actions.className = "db-actions";

  const switchBtn = document.createElement("button");
  switchBtn.type = "button";
  switchBtn.className = "btn btn-switch";
  switchBtn.textContent = "Switch";
  switchBtn.disabled = db.active;
  switchBtn.addEventListener("click", async () => {
    try {
      setStatus(`Switching to ${db.name}...`);
      await switchDatabase(db.name);
      setStatus(`Switched to ${db.name}.`);
      await loadDatabases();
    } catch (err) {
      console.error("Switch failed", err);
      setStatus(err.message || "Failed to switch database.", true);
    }
  });

  const delBtn = document.createElement("button");
  delBtn.type = "button";
  delBtn.className = "btn btn-delete";
  delBtn.textContent = "Delete";
  delBtn.disabled = db.active;
  delBtn.addEventListener("click", async () => {
    try {
      setStatus(`Deleting ${db.name}...`);
      await deleteDatabase(db.name);
      setStatus(`Deleted ${db.name}.`);
      await loadDatabases();
    } catch (err) {
      console.error("Delete failed", err);
      setStatus(err.message || "Failed to delete database.", true);
    }
  });

  actions.append(switchBtn, delBtn);

  row.addEventListener("dblclick", (evt) => {
    if (evt.target.closest("button")) return;
    openDraft({ mode: "edit", name: db.name });
  });

  row.append(meta, actions);
  return row;
};

const render = () => {
  const term = searchInput.value.trim().toLowerCase();
  listEl.innerHTML = "";
  const fragment = document.createDocumentFragment();

  const sorted = sortDatabases(databases).filter((db) =>
    db.name.toLowerCase().includes(term)
  );

  const shouldShowCreateDraft = draftState.open && draftState.mode === "create";
  if (shouldShowCreateDraft) {
    fragment.appendChild(
      buildDraftRow({
        initialName: draftState.initialName,
        onSubmit: async ({ name }) => {
          const finalName = withDbExt(stripDbExt(name));
          const dup = databases.find((d) => d.name.toLowerCase() === finalName.toLowerCase());
          if (dup) throw new Error("A database with that name already exists.");
          setStatus("Creating database...");
          await createDatabase(finalName);
          setStatus(`Created ${finalName}.`);
          closeDraft();
          await loadDatabases();
        },
        onCancel: () => {
          closeDraft();
          render();
        }
      })
    );
  }

  sorted.forEach((db) => {
    if (draftState.open && draftState.mode === "edit" && draftState.editingName === db.name) {
      fragment.appendChild(
        buildDraftRow({
          initialName: draftState.initialName,
          onSubmit: async ({ name }) => {
            const finalName = withDbExt(stripDbExt(name));
            const currentName = db.name;
            if (finalName === currentName) {
              closeDraft();
              render();
              return;
            }
            const dup = databases.find(
              (d) => d.name.toLowerCase() === finalName.toLowerCase()
            );
            if (dup) {
              throw new Error("A database with that name already exists.");
            }
            setStatus("Renaming database...");
            await renameDatabase(currentName, finalName);
            setStatus(`Renamed to ${finalName}.`);
            closeDraft();
            await loadDatabases();
          },
          onCancel: () => {
            closeDraft();
            render();
          }
        })
      );
    } else {
      fragment.appendChild(buildRow(db));
    }
  });

  listEl.appendChild(fragment);
  emptyEl.classList.toggle("hidden", draftState.open || sorted.length > 0);
};

createBtn.addEventListener("click", () => {
  if (draftState.open) {
    const existingInput = document.querySelector(".db-name-input");
    if (existingInput) existingInput.focus();
    return;
  }
  openDraft({ mode: "create" });
});

searchInput.addEventListener("input", render);

document.addEventListener(
  "pointerdown",
  (evt) => {
    if (!draftState.open || !currentDraftForm) return;
    if (currentDraftForm.contains(evt.target)) return;
    currentDraftForm.requestSubmit();
  },
  true
);

const loadDatabases = async () => {
  setStatus("Loading databases...");
  try {
    const list = await fetchDatabases();
    databases = (list || []).map((db) => ({
      name: db.name || "",
      active: db.active === true
    }));
    setStatus(`Loaded ${databases.length} database(s).`);
  } catch (err) {
    console.error("Failed to load databases", err);
    setStatus(err.message || "Failed to load databases.", true);
    databases = [];
  }
  render();
};

loadDatabases();
render();
