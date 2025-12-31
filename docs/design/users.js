import { fetchUsers, fetchUserRoles, createUser, updateUser, deleteUser } from "./api.js";

const createBtn = document.getElementById("create-user-btn");
const searchInput = document.getElementById("user-search");
const userListEl = document.getElementById("user-list");
const emptyEl = document.getElementById("user-empty");
const statusEl = document.getElementById("user-status");
let currentDraftForm = null;

document.querySelectorAll(".nav-btn[data-target]").forEach((btn) => {
  btn.addEventListener("click", () => {
    const target = btn.dataset.target;
    if (target) {
      window.location.href = target;
    }
  });
});

let users = [];

let draftState = {
  open: false,
  mode: "create",
  editingId: null,
  initialName: "",
  initialRole: "Developer"
};

const setStatus = (msg, isError = false) => {
  if (!statusEl) return;
  statusEl.textContent = msg || "";
  statusEl.classList.toggle("error", Boolean(isError));
};

const toUser = (dto = {}) => {
  const name = dto.name || dto.id || dto.username || "";
  return {
    id: name,
    name: name,
    role: dto.role || "Developer"
  };
};

const defaultRoles = ["Developer", "Owner", "Admin", "Viewer"];
let roles = defaultRoles.slice();

const getRoles = () => (roles && roles.length ? roles : defaultRoles);

const buildUserRow = (user) => {
  const row = document.createElement("div");
  row.className = "user-row";

  const meta = document.createElement("div");
  meta.className = "user-meta";
  const strong = document.createElement("strong");
  strong.textContent = user.name;
  const role = document.createElement("span");
  role.className = "role";
  role.textContent = user.role;
  meta.append(strong, role);

  const del = document.createElement("button");
  del.type = "button";
  del.className = "btn btn-delete user-delete";
  del.textContent = "Delete";
  del.addEventListener("click", async () => {
    try {
      await deleteUser(user.id);
      users = users.filter((u) => u.id !== user.id);
      setStatus(`Deleted ${user.name}.`);
      renderUsers();
    } catch (err) {
      console.error("Failed to delete user", err);
      setStatus(err.message || "Failed to delete user.", true);
    }
  });

  row.addEventListener("dblclick", (evt) => {
    if (evt.target.closest("button")) return;
    openDraft({
      mode: "edit",
      user
    });
  });

  row.append(meta, del);
  return row;
};

const buildDraftRow = ({ initialName, initialRole, onSubmit, onCancel }) => {
  const form = document.createElement("form");
  form.className = "user-row draft";
  currentDraftForm = form;

  const fields = document.createElement("div");
  fields.className = "user-draft-fields";

  const nameInput = document.createElement("input");
  nameInput.type = "text";
  nameInput.placeholder = "New User";
  nameInput.className = "user-name-input";
  nameInput.required = true;
  nameInput.value = initialName || "";

  const roleSelect = document.createElement("select");
  roleSelect.className = "user-role-select";
  const roleOptions = getRoles();
  roleOptions.forEach((role) => {
    const opt = document.createElement("option");
    opt.value = role;
    opt.textContent = role;
    roleSelect.appendChild(opt);
  });
  roleSelect.value = initialRole || "Developer";

  fields.append(nameInput, roleSelect);

  const cancel = document.createElement("button");
  cancel.type = "button";
  cancel.className = "btn user-cancel";
  cancel.textContent = "Cancel";
  cancel.addEventListener("click", () => {
    onCancel();
  });

  form.append(fields, cancel);

  form.addEventListener("submit", async (evt) => {
    evt.preventDefault();
    const name = nameInput.value.trim();
    if (!name) {
      nameInput.focus();
      return;
    }
    try {
      await onSubmit({
        name,
        role: roleSelect.value
      });
    } catch (err) {
      console.error("User draft submit failed", err);
      setStatus(err.message || "Failed to save user.", true);
    }
  });

  setTimeout(() => nameInput.focus(), 0);

  return form;
};

const openDraft = ({ mode, user }) => {
  draftState = {
    open: true,
    mode,
    editingId: user?.id || null,
    initialName: user?.name || "",
    initialRole: user?.role || "Developer"
  };
  renderUsers();
};

const closeDraft = () => {
  draftState = {
    open: false,
    mode: "create",
    editingId: null,
    initialName: "",
    initialRole: "Developer"
  };
  currentDraftForm = null;
};

const renderUsers = () => {
  const term = searchInput.value.trim().toLowerCase();
  userListEl.innerHTML = "";
  const fragment = document.createDocumentFragment();

  const filtered = users.filter((user) => {
    const text = `${user.name} ${user.role}`.toLowerCase();
    return text.includes(term);
  });

  const shouldShowCreateDraft = draftState.open && draftState.mode === "create";
  if (shouldShowCreateDraft) {
    fragment.appendChild(
      buildDraftRow({
        initialName: draftState.initialName,
        initialRole: draftState.initialRole,
        onSubmit: async ({ name, role }) => {
          const existing = users.find((u) => u.name.toLowerCase() === name.toLowerCase());
          if (existing) {
            throw new Error("A user with that name already exists.");
          }
          setStatus("Saving user...");
          const created = await createUser({ name, role });
          users = [toUser(created), ...users];
          searchInput.value = "";
          closeDraft();
          setStatus(`Created ${name}.`);
          renderUsers();
        },
        onCancel: () => {
          closeDraft();
          renderUsers();
        }
      })
    );
  }

  filtered.forEach((user) => {
    if (draftState.open && draftState.mode === "edit" && draftState.editingId === user.id) {
      fragment.appendChild(
        buildDraftRow({
          initialName: draftState.initialName,
          initialRole: draftState.initialRole,
        onSubmit: async ({ name, role }) => {
          const current = users.find((u) => u.id === user.id);
          const updates = {};
          if (current && current.name !== name) updates.name = name;
          if (current && current.role !== role) updates.role = role;

          if (Object.keys(updates).length === 0) {
            closeDraft();
            renderUsers();
            return;
          }

          if (updates.name) {
            const dup = users.find(
              (u) => u.id !== user.id && u.name.toLowerCase() === updates.name.toLowerCase()
            );
            if (dup) {
              throw new Error("Another user already has that name.");
            }
          }

          setStatus("Saving changes...");
          await updateUser(user.id, updates);
          users = users.map((u) =>
            u.id === user.id ? { ...u, name, role, ...(updates.name ? { id: name } : {}) } : u
          );
          closeDraft();
          setStatus(`Updated ${user.name}.`);
          renderUsers();
        },
          onCancel: () => {
            closeDraft();
            renderUsers();
          }
        })
      );
    } else {
      fragment.appendChild(buildUserRow(user));
    }
  });

  userListEl.appendChild(fragment);
  emptyEl.classList.toggle("hidden", draftState.open || filtered.length > 0);
};

createBtn.addEventListener("click", () => {
  if (draftState.open) {
    const existingInput = document.querySelector(".user-name-input");
    if (existingInput) existingInput.focus();
    return;
  }
  openDraft({ mode: "create" });
});

searchInput.addEventListener("input", renderUsers);

document.addEventListener(
  "pointerdown",
  (evt) => {
    if (!draftState.open || !currentDraftForm) return;
    if (currentDraftForm.contains(evt.target)) return;
    // Attempt to save when clicking away; form submit handler validates.
    currentDraftForm.requestSubmit();
  },
  true
);

const loadUsers = async () => {
  setStatus("Loading users...");
  try {
    const list = await fetchUsers();
    users = (list || []).map(toUser);
    setStatus(`Loaded ${users.length} user(s).`);
  } catch (err) {
    console.error("Failed to load users", err);
    setStatus(err.message || "Failed to load users.", true);
    users = [];
  }
  renderUsers();
};

const loadRoles = async () => {
  try {
    const list = await fetchUserRoles();
    if (Array.isArray(list) && list.length) {
      roles = list.map((r) => `${r}`.trim()).filter(Boolean);
      setStatus(`Loaded roles: ${roles.join(", ")}`);
      if (draftState.open) {
        renderUsers();
      }
    }
  } catch (err) {
    console.error("Failed to load roles", err);
    // keep default roles; no status error so we don't overwrite user messages
  }
};

loadRoles();
loadUsers();
renderUsers();
