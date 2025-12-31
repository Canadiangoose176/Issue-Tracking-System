import {
  modalBackdropTemplate,
  modalTemplate,
  pillTemplate,
  commentCardTemplate,
  commentFormTemplate,
  tagChipTemplate,
  tagsPillTemplate,
  tagManagerTemplate,
  tagManagerRowTemplate,
  tagEditorFormTemplate
} from "./templates.js";
import { patchIssueFields, fetchUsers, createIssue, mapIssue, apiClient } from "./api.js";
import { tagStore } from "./store.js";

const STATUS_OPTIONS = ["To Be Done", "In Progress", "Done"];

const normalizeStatusValue = (value) => {
  if (value === undefined || value === null) return "";
  const trimmed = `${value}`.trim();
  if (!trimmed) return "";
  const lower = trimmed.toLowerCase();
  if (trimmed === "1" || lower === "todo" || lower === "to be done") {
    return STATUS_OPTIONS[0];
  }
  if (trimmed === "2" || lower === "in progress") {
    return STATUS_OPTIONS[1];
  }
  if (trimmed === "3" || lower === "done") {
    return STATUS_OPTIONS[2];
  }
  return trimmed;
};

const pillConfig = [
  { label: "Assignees", className: "assignees", value: (issue) => issue.assignedTo || "" },
  { label: "Tags", className: "tags", value: (issue) => (issue.tags || []).map((t) => t.label).join(", ") },
  {
    label: "Status",
    className: "status",
    value: (issue) => normalizeStatusValue(issue.status || "")
  }
];

const setText = (el, value, fallback = "") => {
  if (!el) return;
  el.textContent = value || fallback;
};

const defaultTagColor = "#49a3d8";
const normalizeTagName = (tag) =>
  ((tag && (tag.label || tag.tag || "")) || "").toString();

const normalizeTags = (tags) =>
  (Array.isArray(tags) ? tags : []).map((t) => ({
    id: t.id,
    label: normalizeTagName(t) || "Tag",
    color: (t && t.color) || defaultTagColor
  }));

const tagKey = (tag) => {
  if (tag && tag.id !== undefined && tag.id !== null) {
    return `id:${tag.id}`;
  }
  return `name:${normalizeTagName(tag).toLowerCase()}`;
};

const updateTagsPill = (modal, tags = []) => {
  const pill = modal.querySelector('[data-role="tags-pill"]');
  if (!pill) return false;
  const wrap = pill.querySelector('[data-role="tags-list"]');
  if (!wrap) return false;
  wrap.replaceChildren();
  const normalized = normalizeTags(tags);
  if (!normalized.length) {
    const empty = document.createElement("div");
    empty.className = "tags-pill__empty";
    empty.textContent = "No tags yet";
    wrap.appendChild(empty);
  } else {
    normalized.forEach((tag) => wrap.appendChild(tagChipTemplate(tag)));
  }
  return true;
};

const emptyIssue = {
  rawId: null,
  id: "",
  title: "",
  description: "",
  status: "To Be Done",
  assignedTo: "",
  comments: [],
  tags: []
};

export const createModal = ({ onIssueUpdated, getActiveDatabase } = {}) => {
  const backdrop = modalBackdropTemplate();
  document.body.appendChild(backdrop);
  let currentIssue = null;
  let workingIssue = null;
  let usersCache = null;
  const reloadAfterStatusChange = () => {
    window.location.reload();
  };

  const setStatus = (modal, message, isError = false) => {
    const box = modal.querySelector('[data-role="form-status"]');
    if (!box) return;
    box.textContent = message || "";
    box.classList.toggle("error", isError);
  };

  const startCommentInlineEdit = (modal, bodyEl, comment, index) => {
    if (!bodyEl || !comment) return;
    const original = comment.text || "";

    bodyEl.contentEditable = "true";
    bodyEl.classList.add("editing-inline");
    setStatus(modal, "Editing comment. Ctrl+Enter to save, Esc to cancel.");

    const selectAll = () => {
      const range = document.createRange();
      range.selectNodeContents(bodyEl);
      const sel = window.getSelection();
      sel.removeAllRanges();
      sel.addRange(range);
    };
    selectAll();

    const cleanup = () => {
      bodyEl.contentEditable = "false";
      bodyEl.classList.remove("editing-inline");
      bodyEl.removeEventListener("keydown", onKeyDown);
      bodyEl.removeEventListener("blur", onBlur);
    };

    const cancel = () => {
      bodyEl.textContent = original;
      cleanup();
      setStatus(modal, "");
    };

    const save = async () => {
      const next = bodyEl.textContent.trim();
      if (next.length === 0) {
        setStatus(modal, "Comment cannot be empty.", true);
        bodyEl.focus();
        return;
      }
      if (next === original) {
        cancel();
        return;
      }

      const addBtn = modal.querySelector(".add-comment");
      if (addBtn) addBtn.disabled = true;

      try {
        setStatus(modal, "Saving comment...");
        if (
          comment.id !== undefined &&
          comment.id !== null &&
          workingIssue?.rawId !== undefined &&
          workingIssue?.rawId !== null
        ) {
          await apiClient.updateComment(workingIssue.rawId, comment.id, next);
        }
        const nextComments = (workingIssue.comments || []).map((c, i) => {
          const matchesById =
            c.id !== undefined &&
            c.id !== null &&
            comment.id !== undefined &&
            comment.id !== null &&
            c.id === comment.id;
          if (matchesById || i === index) {
            return { ...c, text: next };
          }
          return c;
        });
        workingIssue = { ...workingIssue, comments: nextComments };
        currentIssue = { ...workingIssue };
        renderComments(modal, workingIssue);
        setStatus(modal, "Comment saved.");
        if (onIssueUpdated) {
          onIssueUpdated(workingIssue);
        }
      } catch (err) {
        setStatus(modal, err.message || "Failed to save comment.", true);
      } finally {
        cleanup();
        if (addBtn) addBtn.disabled = false;
      }
    };

    const onKeyDown = (evt) => {
      if (evt.key === "Escape") {
        evt.preventDefault();
        cancel();
      } else if (evt.key === "Enter" && (evt.ctrlKey || evt.metaKey)) {
        evt.preventDefault();
        bodyEl.blur();
      }
    };

    const onBlur = () => {
      void save();
    };

    bodyEl.addEventListener("keydown", onKeyDown);
    bodyEl.addEventListener("blur", onBlur, { once: true });
  };

  const handleDeleteComment = async (modal, comment, index) => {
    if (!workingIssue || workingIssue.rawId === undefined || workingIssue.rawId === null) {
      setStatus(modal, "Save the issue before deleting comments.", true);
      return;
    }

    const addBtn = modal.querySelector(".add-comment");
    if (addBtn) addBtn.disabled = true;

    try {
      if (comment.id !== undefined && comment.id !== null) {
        setStatus(modal, "Deleting comment...");
        await apiClient.deleteComment(workingIssue.rawId, comment.id);
      } else {
        setStatus(modal, "Removing comment locally (no id).");
      }
      const nextComments = (workingIssue.comments || []).filter((c, i) => {
        const matchesById =
          c.id !== undefined &&
          c.id !== null &&
          comment.id !== undefined &&
          comment.id !== null &&
          c.id === comment.id;
        return !(matchesById || i === index);
      });
      workingIssue = { ...workingIssue, comments: nextComments };
      currentIssue = { ...workingIssue };
      renderComments(modal, workingIssue);
      setStatus(modal, "Comment deleted.");
      if (onIssueUpdated) {
        onIssueUpdated(workingIssue);
      }
    } catch (err) {
      setStatus(modal, err.message || "Failed to delete comment.", true);
    } finally {
      if (addBtn) addBtn.disabled = false;
    }
  };

  const renderComments = (modal, issue) => {
    const commentsWrap = modal.querySelector('[data-role="comments"]');
    if (!commentsWrap) return;
    commentsWrap.replaceChildren();
    (issue.comments || []).forEach((comment, idx) => {
      const card = commentCardTemplate(comment);
      const body = card.querySelector(".comment-body");
      const deleteBtn = card.querySelector(".comment-delete");
      body?.addEventListener("dblclick", () => {
        startCommentInlineEdit(modal, body, comment, idx);
      });
      deleteBtn?.addEventListener("click", (evt) => {
        evt.preventDefault();
        void handleDeleteComment(modal, comment, idx);
      });
      commentsWrap.appendChild(card);
    });
  };

  const loadUsers = async () => {
    if (usersCache) return usersCache;
    const users = await fetchUsers();
    usersCache = users;
    return users;
  };

  const normalizeUser = (u) => (u && u.name) || u?.id || u || "";

  const findUserByName = (users, name) => {
    if (!name) return null;
    const target = name.toLowerCase();
    return (users || []).find((u) => normalizeUser(u).toLowerCase() === target) || null;
  };

  const startAssigneeEdit = async (modal, pillEl) => {
    if (!workingIssue || workingIssue.rawId === undefined || workingIssue.rawId === null) {
      setStatus(modal, "Save the issue before assigning.", true);
      return;
    }

    const originalText = pillEl.textContent;
    pillEl.classList.add("editing-assignee");
    pillEl.textContent = "";
    const input = document.createElement("input");
    input.type = "text";
    input.placeholder = "Enter user to assign";
    input.value = workingIssue.assignedTo || "";
    input.classList.add("assignee-input");
    pillEl.appendChild(input);
    input.focus();
    input.select();

    const restoreLabel = () => {
      pillEl.classList.remove("editing-assignee");
      pillEl.textContent = originalText;
    };

    const applyLabel = (assignee) => {
      pillEl.classList.remove("editing-assignee");
      pillEl.textContent = assignee ? `Assignees: ${assignee}` : "Assignees";
    };

    const onCancel = () => {
      restoreLabel();
      setStatus(modal, "");
    };

    const commit = async () => {
      const value = (input.value || "").trim();
      let users = [];
      if (value) {
        try {
          users = await loadUsers();
        } catch (err) {
          setStatus(modal, err.message || "Failed to load users.", true);
          input.focus();
          input.select();
          return;
        }
      }
      const match = findUserByName(users, value);

      if (!value) {
        try {
          setStatus(modal, "Unassigning...");
          const updated = await apiClient.unassignIssue(workingIssue.rawId);
          const dbName =
            (getActiveDatabase && getActiveDatabase()) || apiClient.getActiveDatabaseName();
          const mapped = mapIssue(updated, dbName);
          workingIssue = { ...workingIssue, ...mapped };
          currentIssue = { ...workingIssue };
          fillView(modal, workingIssue);
          setStatus(modal, "Issue unassigned.");
          if (onIssueUpdated) {
            onIssueUpdated(workingIssue);
          }
        } catch (err) {
          setStatus(modal, err.message || "Failed to unassign.", true);
          restoreLabel();
        }
        return;
      }

      if (!match) {
        setStatus(modal, "User not found. Try another name.", true);
        input.focus();
        input.select();
        return;
      }

      if (value === workingIssue.assignedTo) {
        applyLabel(value);
        setStatus(modal, "");
        return;
      }

      try {
        setStatus(modal, "Assigning user...");
        const updated = await apiClient.assignUserToIssue(workingIssue.rawId, value);
        const dbName =
          (getActiveDatabase && getActiveDatabase()) || apiClient.getActiveDatabaseName();
        const mapped = mapIssue(updated, dbName);
        workingIssue = { ...workingIssue, ...mapped };
        currentIssue = { ...workingIssue };
        fillView(modal, workingIssue);
        setStatus(modal, `Assignee set to ${workingIssue.assignedTo || value}.`);
        if (onIssueUpdated) {
          onIssueUpdated(workingIssue);
        }
      } catch (err) {
        setStatus(modal, err.message || "Failed to assign user.", true);
        restoreLabel();
      }
    };

    const onKeyDown = (evt) => {
      if (evt.key === "Escape") {
        evt.preventDefault();
        onCancel();
      } else if (evt.key === "Enter") {
        evt.preventDefault();
        void commit();
      }
    };

    const onBlur = () => {
      void commit();
    };

    input.addEventListener("keydown", onKeyDown);
    input.addEventListener("blur", onBlur, { once: true });
  };

  const startStatusEdit = async (modal, pillEl) => {
    if (!workingIssue || workingIssue.rawId === undefined || workingIssue.rawId === null) {
      setStatus(modal, "Save the issue before changing status.", true);
      return;
    }

    const originalText = pillEl.textContent;
    pillEl.classList.add("editing-status");
    pillEl.textContent = "";

    const select = document.createElement("select");
    select.classList.add("status-select");

    STATUS_OPTIONS.forEach((label) => {
      const opt = document.createElement("option");
      opt.value = label;
      opt.textContent = label;
      select.appendChild(opt);
    });

    const current = normalizeStatusValue(
        workingIssue.status || ""
      );
    if (current) {
      select.value = current;
    }

    pillEl.appendChild(select);
    select.focus();

    const restoreLabel = () => {
      pillEl.classList.remove("editing-status");
      pillEl.textContent = originalText;
    };

    const applyLabel = (status) => {
      pillEl.classList.remove("editing-status");
      pillEl.textContent = status ? `Status: ${status}` : "Status";
    };

    const commit = async () => {
      const selected = normalizeStatusValue(select.value);
      const previous = normalizeStatusValue(
        workingIssue.status || ""
      );
      if (!selected || selected === previous) {
        restoreLabel();
        setStatus(modal, "");
        return;
      }

      try {
        setStatus(modal, "Updating status...");
        await patchIssueFields(workingIssue.rawId, { status: selected });
        workingIssue = { ...workingIssue, status: selected };
        currentIssue = { ...workingIssue };
        applyLabel(selected);
        if (onIssueUpdated) {
          onIssueUpdated(workingIssue);
        }
        setStatus(modal, "");
        reloadAfterStatusChange();
      } catch (err) {
        setStatus(modal, err.message || "Failed to update status.", true);
        select.focus();
      }
    };

    const onKeyDown = (evt) => {
      if (evt.key === "Escape") {
        evt.preventDefault();
        restoreLabel();
        setStatus(modal, "");
      } else if (evt.key === "Enter") {
        evt.preventDefault();
        void commit();
      }
    };

    const onBlur = () => {
      void commit();
    };

    select.addEventListener("keydown", onKeyDown);
    select.addEventListener("blur", onBlur);
  };

  const getTagManagerHost = (modal) => {
    const sidebar = modal.querySelector('[data-role="sidebar"]');
    if (!sidebar) return null;
    const tagsPill = sidebar.querySelector('[data-role="tags-pill"]');
    if (!tagsPill) return null;
    let host = tagsPill.nextElementSibling;
    if (!host || host.dataset.role !== "tag-manager-inline") {
      host = document.createElement("div");
      host.dataset.role = "tag-manager-inline";
      host.classList.add("hidden");
      tagsPill.insertAdjacentElement("afterend", host);
    }
    return host;
  };

  const closeTagManager = (modal) => {
    const host = getTagManagerHost(modal);
    if (host) {
      host.classList.add("hidden");
      host.innerHTML = "";
    }
    const legacyRegion = modal.querySelector('[data-role="tag-manager-region"]');
    if (legacyRegion) {
      legacyRegion.classList.add("hidden");
      legacyRegion.innerHTML = "";
    }
  };

  const isTagManagerOpen = (modal) => {
    const host = getTagManagerHost(modal);
    return !!(host && !host.classList.contains("hidden"));
  };

  const renderSidebar = (modal, issue) => {
    const sidebar = modal.querySelector('[data-role="sidebar"]');
    if (!sidebar) return;
    sidebar.replaceChildren();
    pillConfig.forEach((pill) => {
      if (pill.className === "tags") {
        const tagsPill = tagsPillTemplate(normalizeTags(issue.tags));
        tagsPill.title = "Double-click to manage tags";
        tagsPill.addEventListener("dblclick", () => {
          void openTagManager(modal);
        });
        tagsPill.addEventListener("keydown", (evt) => {
          if (evt.key === "Enter" || evt.key === " ") {
            evt.preventDefault();
            void openTagManager(modal);
          }
        });
        sidebar.appendChild(tagsPill);
        return;
      }
      const el = pillTemplate(pill.label, pill.className);
      const value = pill.value ? pill.value(issue) : "";
      el.textContent = value ? `${pill.label}: ${value}` : pill.label;
      sidebar.appendChild(el);
    });
    const assigneePill = sidebar.querySelector(".detail-pill.assignees");
    assigneePill?.addEventListener("dblclick", () => {
      void startAssigneeEdit(modal, assigneePill);
    });

    const statusPill = sidebar.querySelector(".detail-pill.status");
    statusPill?.setAttribute("title", "Double-click to change status");
    statusPill?.addEventListener("dblclick", () => {
      void startStatusEdit(modal, statusPill);
    });
    statusPill?.addEventListener("keydown", (evt) => {
      if (evt.key === "Enter" || evt.key === " ") {
        evt.preventDefault();
        void startStatusEdit(modal, statusPill);
      }
    });
  };

  const openTagManager = async (modal) => {
    if (!workingIssue || workingIssue.rawId === undefined || workingIssue.rawId === null) {
      setStatus(modal, "Save the issue before managing tags.", true);
      return;
    }

    const host = getTagManagerHost(modal);
    if (!host) return;

    host.innerHTML = "";
    const manager = tagManagerTemplate();
    host.appendChild(manager);
    host.classList.remove("hidden");

    const listEl = manager.querySelector('[data-role="tag-list"]');
    const searchInput = manager.querySelector('[data-role="tag-search"]');
    const closeBtn = manager.querySelector('[data-role="tag-manager-close"]');
    const createTrigger = manager.querySelector('[data-role="open-tag-create"]');
    const createHost = manager.querySelector('[data-role="tag-create-host"]');
    let selectedKey = "";
    let activeForm = null;
    let activeMode = null;
    let assignedSet = new Set(normalizeTags(workingIssue.tags).map((t) => tagKey(t)));
    let availableTags = [];

    if (createTrigger) {
      createTrigger.title = "Double-click to create a tag";
    }

    const destroyForm = () => {
      if (activeForm && activeForm.parentNode) {
        activeForm.parentNode.removeChild(activeForm);
      }
      activeForm = null;
      activeMode = null;
      selectedKey = "";
      listEl?.querySelectorAll(".tag-manager__row").forEach((row) =>
        row.classList.remove("is-selected")
      );
    };

    const syncFromAvailable = () => {
      const assigned = availableTags.filter((t) => t.assigned);
      workingIssue = { ...workingIssue, tags: assigned };
      currentIssue = { ...workingIssue };
      updateTagsPill(modal, workingIssue.tags);
      if (onIssueUpdated) {
        onIssueUpdated(workingIssue);
      }
    };

    const seedDefinitions = (defs = []) => {
      const normalizedDefs = normalizeTags(defs);
      normalizedDefs.forEach((tag) => {
        const key = tagKey(tag);
        const existingIdx = availableTags.findIndex(
          (t) => tagKey(t) === key
        );
        const assigned =
          assignedSet.has(key) ||
          (existingIdx >= 0 && availableTags[existingIdx].assigned);
        const next = { ...tag, assigned };
        if (existingIdx >= 0) {
          availableTags[existingIdx] = { ...availableTags[existingIdx], ...next };
        } else {
          availableTags.push(next);
        }
      });
    };

    const applyServerTags = (latest = []) => {
      const normalized = normalizeTags(latest);
      const map = new Map();
      availableTags.forEach((t) => map.set(tagKey(t), { ...t, assigned: false }));
      normalized.forEach((t) => {
        const key = tagKey(t);
        map.set(key, { ...(map.get(key) || {}), ...t, assigned: true });
      });
      availableTags = Array.from(map.values());
      assignedSet = new Set(
        availableTags.filter((t) => t.assigned).map((t) => tagKey(t))
      );
      tagStore.setDefinitions(normalized);
      syncFromAvailable();
    };

    const handleFormSubmit = async (evt, nameInput, colorInput, originalTag = null) => {
      evt.preventDefault();
      const tagName = (nameInput?.value || "").trim();
      const color = colorInput?.value || "";
      if (!tagName) {
        setStatus(modal, "Tag name is required.", true);
        nameInput?.focus();
        return;
      }
      const tagId = originalTag && originalTag.id !== undefined ? originalTag.id : null;

      try {
        setStatus(modal, "Saving tag...");
        if (activeMode === "edit" && tagId) {
          const updatedDef = await apiClient.updateTagDefinition(tagId, { tag: tagName, color });
          const key = tagKey(updatedDef);
          availableTags = availableTags.map((t) =>
            tagKey(t) === key ? { ...t, ...updatedDef } : t
          );
          assignedSet = new Set(
            availableTags.filter((t) => t.assigned).map((t) => tagKey(t))
          );
          // Push the new definition into the current issue + list without waiting for a refetch.
          syncFromAvailable();
        } else {
          const updatedTags = await apiClient.addTagToIssue(workingIssue.rawId, {
            tag: tagName,
            color
          });
          applyServerTags(updatedTags);
        }
        renderList(searchInput?.value || "");
        destroyForm();
        setStatus(modal, `Tag "${tagName}" saved.`);
      } catch (err) {
        setStatus(modal, err.message || "Failed to save tag.", true);
      }
    };

    const showForm = ({ tag = null, anchor = null, mode = "edit" } = {}) => {
      destroyForm();
      const form = tagEditorFormTemplate();
      activeForm = form;
      activeMode = mode;
      const tagNameInput = form.querySelector('[data-field="tag-name"]');
      const tagColorInput = form.querySelector('[data-field="tag-color"]');
      const cancelBtn = form.querySelector('[data-role="cancel-tag-create"]');
      const normalized = tag ? normalizeTags([tag])[0] : { label: "", color: defaultTagColor };
      const initialTag = tag ? { ...normalized, id: tag.id } : null;
      if (tagNameInput) {
        tagNameInput.value = normalized.label || "";
      }
      if (tagColorInput) {
        tagColorInput.value = normalized.color || defaultTagColor;
      }
      selectedKey = tag ? tagKey(tag) : "";
      const target = anchor || createHost;
      if (target) {
        if (target.dataset && target.dataset.role === "tag-create-host") {
          target.replaceChildren(form);
        } else {
          target.insertAdjacentElement("afterend", form);
        }
      } else {
        manager.appendChild(form);
      }
      tagNameInput?.focus();
      tagNameInput?.select();

      cancelBtn?.addEventListener("click", (evt) => {
        evt.preventDefault();
        destroyForm();
      });
      form.addEventListener("submit", (evt) => {
        void handleFormSubmit(evt, tagNameInput, tagColorInput, initialTag);
      });
    };

    const handleRemove = async (tag) => {
      const name = normalizeTagName(tag);
      if (!name) return;
      try {
        setStatus(modal, `Removing ${name}...`);
        await apiClient.removeTagFromIssue(workingIssue.rawId, name);
        availableTags = availableTags.map((t) =>
          tagKey(t) === tagKey(tag)
            ? { ...t, assigned: false }
            : t
        );
        const latest = await apiClient.fetchIssueTags(workingIssue.rawId);
        applyServerTags(latest);
        renderList(searchInput?.value || "");
        setStatus(modal, `Removed ${name}.`);
      } catch (err) {
        setStatus(modal, err.message || "Failed to remove tag.", true);
      }
    };

    const handleDeleteDefinition = async (tag) => {
      const id = tag?.id;
      const name = normalizeTagName(tag) || "tag";
      if (!id) {
        setStatus(modal, "Cannot delete tag without an id.", true);
        return;
      }
      try {
        setStatus(modal, `Deleting ${name}...`);
        await apiClient.deleteTagDefinition(id);
        availableTags = availableTags.filter((t) => tagKey(t) !== tagKey(tag));
        assignedSet.delete(tagKey(tag));
        tagStore.removeDefinition(id);
        const latest = await apiClient.fetchIssueTags(workingIssue.rawId);
        applyServerTags(latest);
        renderList(searchInput?.value || "");
        setStatus(modal, `Deleted ${name}.`);
      } catch (err) {
        setStatus(modal, err.message || "Failed to delete tag.", true);
      }
    };

    const renderList = (filterText = "") => {
      if (!listEl) return;
      if (activeForm && activeMode === "edit") {
        destroyForm();
      }
      const filter = (filterText || "").toLowerCase().trim();
      listEl.replaceChildren();
      const tags = availableTags;
      const filtered = filter
        ? tags.filter((t) => t.label.toLowerCase().includes(filter))
        : tags;
      if (!filtered.length) {
        const empty = document.createElement("div");
        empty.className = "tag-manager__empty";
        empty.textContent = filter
          ? "No tags match your search."
          : "No tags yet. Add one below.";
        listEl.appendChild(empty);
        return;
      }
      filtered.forEach((tag) => {
        const row = tagManagerRowTemplate(tag);
        const deleteBtn = row.querySelector(".tag-manager__delete");
        const chip = row.querySelector(".tag-manager__chip");
        const normalizedName = normalizeTagName(tag);
        if (tag.assigned) {
          row.classList.add("is-selected");
          row.dataset.assigned = "true";
        } else {
          row.dataset.assigned = "false";
        }
        row.addEventListener("click", (evt) => {
          if (evt.target && evt.target.closest(".tag-manager__chip")) {
            return;
          }
          selectedKey = tagKey(tag);
          listEl.querySelectorAll(".tag-manager__row").forEach((r) =>
            r.classList.remove("is-selected")
          );
          row.classList.add("is-selected");
          if (row.dataset.assigned === "true") {
            void handleRemove(tag);
          } else {
            void handleAdd(tag);
          }
        });
        chip?.addEventListener("click", (evt) => {
          evt.preventDefault();
          evt.stopPropagation();
          selectedKey = tagKey(tag);
          listEl.querySelectorAll(".tag-manager__row").forEach((r) =>
            r.classList.remove("is-selected")
          );
          row.classList.add("is-selected");
          showForm({ tag, anchor: row, mode: "edit" });
        });
        deleteBtn?.addEventListener("click", (evt) => {
          evt.stopPropagation();
          void handleDeleteDefinition(tag);
        });
        listEl.appendChild(row);
      });
    };

    const handleAdd = async (tag) => {
      const name = normalizeTagName(tag);
      if (!name) return;
      try {
        setStatus(modal, `Adding ${name}...`);
        const updated = await apiClient.addTagToIssue(workingIssue.rawId, {
          tag: name,
          color: tag.color || defaultTagColor
        });
        applyServerTags(updated);
        renderList(searchInput?.value || "");
        setStatus(modal, `Added ${name}.`);
      } catch (err) {
        setStatus(modal, err.message || "Failed to add tag.", true);
      }
    };

    searchInput?.addEventListener("input", () => {
      renderList(searchInput.value);
    });
    createTrigger?.addEventListener("dblclick", () => {
      showForm({ tag: null, anchor: createHost, mode: "create" });
    });
    closeBtn?.addEventListener("click", () => {
      closeTagManager(modal);
    });

    // Seed with currently assigned tags so something renders immediately.
    seedDefinitions(workingIssue.tags);
    renderList(searchInput?.value || "");

    // Load all tag definitions so users can re-use across issues.
    apiClient
      .fetchAllTags()
      .then((allDefs) => {
        seedDefinitions(allDefs);
        renderList(searchInput?.value || "");
      })
      .catch((err) => {
        console.error("Failed to load all tags", err);
      });

    try {
      setStatus(modal, "Loading tags...");
      const tags = await apiClient.fetchIssueTags(workingIssue.rawId);
      applyServerTags(tags);
      renderList(searchInput?.value || "");
      setStatus(modal, "");
    } catch (err) {
      setStatus(modal, err.message || "Failed to load tags.", true);
    }
  };

  const fillView = (modal, issue) => {
    const heading = issue.id ? `${issue.title} (${issue.id})` : issue.title || "Issue";
    setText(modal.querySelector('[data-field="heading"]'), heading, "Issue");
    setText(modal.querySelector('[data-field="titleBox"]'), heading, "Issue");
    setText(
      modal.querySelector('[data-field="description"]'),
      issue.description,
      "No description provided."
    );
    renderSidebar(modal, issue);
    updateTagsPill(modal, issue.tags);
    renderComments(modal, issue);
  };

  const bindEditHandlers = (modal) => {
    const titleBox = modal.querySelector('[data-field="titleBox"]');
    const heading = modal.querySelector('[data-field="heading"]');
    const descriptionBox = modal.querySelector('[data-field="description"]');
    const authorWrap = modal.querySelector('[data-role="author-select"]');

    const populateAuthor = (users) => {
      if (!authorWrap) return;
      authorWrap.innerHTML = "";

      if (!Array.isArray(users) || users.length === 0) {
        authorWrap.textContent = "No users available. Please add a user.";
        authorWrap.classList.add("error");
        return;
      }

      const label = document.createElement("span");
      label.textContent = "Author";

      // Existing issues should not allow author changes; render as plain text.
      if (workingIssue.rawId !== undefined && workingIssue.rawId !== null) {
        const value = document.createElement("span");
        value.className = "author-readonly";
        value.textContent =
          workingIssue.author ||
          workingIssue.authorId ||
          users[0].name ||
          users[0].id ||
          users[0];
        authorWrap.append(label, value);
        return;
      }

      const select = document.createElement("select");
      users.forEach((u) => {
        const name = u.name || u.id || u;
        const opt = document.createElement("option");
        opt.value = name;
        opt.textContent = name;
        select.appendChild(opt);
      });

      const currentAuthor = workingIssue.author || workingIssue.authorId;
      const defaultAuthor = users[0].name || users[0].id || users[0];
      const resolvedAuthor = currentAuthor || defaultAuthor;
      workingIssue.author = resolvedAuthor;
      select.value = resolvedAuthor;

      select.addEventListener("change", (evt) => {
        workingIssue.author = evt.target.value;
        setStatus(modal, "Pending save (applies when closing).");
      });

      authorWrap.appendChild(label);
      authorWrap.appendChild(select);
    };

    const startInlineEdit = (field, el) => {
      if (!el) return;

      const original =
        field === "title" ? workingIssue.title || "" : workingIssue.description || "";

      // Temporarily show raw value (without the id suffix) for title edits.
      if (field === "title") {
        el.textContent = original;
        setText(heading, original || "Issue");
      } else {
        el.textContent = original;
      }

      el.contentEditable = "true";
      el.classList.add("editing-inline");
      setStatus(modal, `Editing ${field}. Changes save when you close.`);
      const selectAll = () => {
        const range = document.createRange();
        range.selectNodeContents(el);
        const sel = window.getSelection();
        sel.removeAllRanges();
        sel.addRange(range);
      };
      selectAll();

      const cleanup = () => {
        el.contentEditable = "false";
        el.classList.remove("editing-inline");
        el.removeEventListener("keydown", onKeyDown);
        el.removeEventListener("blur", onBlur);
      };

      const cancel = () => {
        cleanup();
        fillView(modal, workingIssue);
        setStatus(modal, "");
      };

      const saveDraft = () => {
        const next = el.textContent.trim();
        if (field === "title" && next.length === 0) {
          setStatus(modal, "Title cannot be empty.", true);
          el.focus();
          return;
        }
        if (next === original) {
          cancel();
          return;
        }

        if (field === "title") {
          workingIssue.title = next;
        } else {
          workingIssue.description = next;
        }
        fillView(modal, workingIssue);
        setStatus(modal, "Pending save (applies when closing).");
        cleanup();
      };

      const onKeyDown = (evt) => {
        if (evt.key === "Escape") {
          evt.preventDefault();
          cancel();
        } else if (evt.key === "Enter" && (field === "title" || evt.ctrlKey)) {
          evt.preventDefault();
          el.blur();
        }
      };

      const onBlur = () => {
        saveDraft();
      };

      el.addEventListener("keydown", onKeyDown);
      el.addEventListener("blur", onBlur, { once: true });
    };

    const startTitleEdit = () => startInlineEdit("title", titleBox);
    const startDescEdit = () => startInlineEdit("description", descriptionBox);
    heading?.addEventListener("dblclick", startTitleEdit);
    titleBox?.addEventListener("dblclick", startTitleEdit);
    descriptionBox?.addEventListener("dblclick", startDescEdit);

    // hydrate author selector once users are loaded
    loadUsers()
      .then((users) => {
        populateAuthor(users);
      })
      .catch((err) => {
        authorWrap.textContent = err.message || "Failed to load users.";
        authorWrap.classList.add("error");
      });
  };

  const openCommentForm = async (modal) => {
    if (!workingIssue || workingIssue.rawId === undefined || workingIssue.rawId === null) {
      setStatus(modal, "Save the issue before adding comments.", true);
      return;
    }

    const existing = modal.querySelector('[data-role="comment-form"]');
    if (existing) {
      existing.querySelector('[data-field="comment-text"]')?.focus();
      return;
    }

    const form = commentFormTemplate();
    const authorSelect = form.querySelector('[data-field="comment-author"]');
    const textArea = form.querySelector('[data-field="comment-text"]');
    const cancelBtn = form.querySelector('[data-role="cancel-comment"]');
    const region = modal.querySelector('[data-role="comment-form-region"]');

    const populateAuthors = (users) => {
      if (!authorSelect) return;
      authorSelect.innerHTML = "";

      if (!Array.isArray(users) || users.length === 0) {
        const fallback = workingIssue.author || workingIssue.authorId || "Author";
        const opt = document.createElement("option");
        opt.value = fallback;
        opt.textContent = fallback;
        authorSelect.appendChild(opt);
        return fallback;
      }

      users.forEach((u) => {
        const name = u.name || u.id || u;
        const opt = document.createElement("option");
        opt.value = name;
        opt.textContent = name;
        authorSelect.appendChild(opt);
      });

      const preferred = workingIssue.author || workingIssue.authorId;
      const defaultAuthor = users[0].name || users[0].id || users[0];
      const resolved = preferred || defaultAuthor;
      authorSelect.value = resolved;
      return resolved;
    };

    populateAuthors(usersCache || [workingIssue.author || workingIssue.authorId || "Author"]);
    loadUsers()
      .then((users) => {
        populateAuthors(users);
      })
      .catch((err) => {
        populateAuthors([]);
        setStatus(modal, err.message || "Failed to load users; using fallback author.", true);
      });

    const onSubmit = async (evt) => {
      evt.preventDefault();
      const text = (textArea?.value || "").trim();
      const authorId = (authorSelect?.value || "").trim();
      if (!text) {
        setStatus(modal, "Comment cannot be empty.", true);
        textArea?.focus();
        return;
      }
      if (!authorId) {
        setStatus(modal, "Author is required for comments.", true);
        authorSelect?.focus();
        return;
      }

      const addBtn = modal.querySelector(".add-comment");
      if (addBtn) addBtn.disabled = true;

      try {
        setStatus(modal, "Posting comment...");
        const comment = await apiClient.createComment(workingIssue.rawId, {
          text,
          authorId
        });
        const nextComments = [...(workingIssue.comments || []), comment];
        workingIssue = { ...workingIssue, comments: nextComments };
        currentIssue = { ...workingIssue };
        renderComments(modal, workingIssue);
        if (region) {
          region.replaceChildren();
        } else {
          form.remove();
        }
        setStatus(modal, "Comment added.");
        if (onIssueUpdated) {
          onIssueUpdated(workingIssue);
        }
      } catch (err) {
        setStatus(modal, err.message || "Failed to add comment.", true);
      } finally {
        if (addBtn) addBtn.disabled = false;
      }
    };

    form.addEventListener("submit", onSubmit);
    cancelBtn?.addEventListener("click", (evt) => {
      evt.preventDefault();
      if (region) {
        region.replaceChildren();
      } else {
        form.remove();
      }
      setStatus(modal, "");
    });

    if (region) {
      region.replaceChildren(form);
    } else {
      modal.appendChild(form);
    }
    textArea?.focus();
  };

  const buildDetail = (issue) => {
    const modal = modalTemplate();
    modal.dataset.issueId = issue.rawId ?? "";
    currentIssue = { ...apiClient.mapIssue(issue) };
    workingIssue = { ...currentIssue };
    workingIssue.status = normalizeStatusValue(workingIssue.status);
    if (!workingIssue.database) {
      workingIssue.database =
        (getActiveDatabase && getActiveDatabase()) || apiClient.getActiveDatabaseName();
    }
    fillView(modal, workingIssue);
    bindEditHandlers(modal);
    modal.querySelector(".add-comment")?.addEventListener("click", () => {
      void openCommentForm(modal);
    });
    const cancelBtn = modal.querySelector('[data-role="cancel-modal"]');
    cancelBtn?.addEventListener("click", () => {
      void close(true); // discard any changes
    });
    modal.querySelector(".modal-close")?.addEventListener("click", () => {
      void close(true);
    });
    setStatus(modal, "");
    return modal;
  };

  const persistChanges = async (modal) => {
    if (!workingIssue) return true;

    const needsCreate = workingIssue.rawId === undefined || workingIssue.rawId === null;
    if (needsCreate) {
      const title = (workingIssue.title || "").trim();
      if (title.length === 0) {
        setStatus(modal, "Title is required to create an issue.", true);
        return false;
      }
      const authorId = (
        workingIssue.author ||
        workingIssue.authorId ||
        (usersCache &&
          usersCache[0] &&
          (usersCache[0].name || usersCache[0].id || usersCache[0])) ||
        ""
      ).trim();
      if (!authorId) {
        setStatus(modal, "Author is required to create an issue.", true);
        return false;
      }
      try {
        setStatus(modal, "Creating issue...");
        const created = await createIssue({
          title,
          description: workingIssue.description || "",
          authorId
        });
        const dbName =
          (getActiveDatabase && getActiveDatabase()) || apiClient.getActiveDatabaseName();
        const mapped = mapIssue(created, dbName);
        workingIssue = {
          ...workingIssue,
          ...mapped,
          rawId: Number(mapped.rawId ?? mapped.id ?? workingIssue.rawId),
          database: mapped.database || dbName,
          author: mapped.author || workingIssue.author,
          status: normalizeStatusValue(mapped.status || workingIssue.status)
        };
        currentIssue = { ...workingIssue };
        if (onIssueUpdated) {
          onIssueUpdated(workingIssue);
        }
        return true;
      } catch (err) {
        setStatus(modal, err.message || "Failed to create issue.", true);
        return false;
      }
    }

    const payload = {};
    if (workingIssue.title !== currentIssue.title) {
      payload.title = workingIssue.title;
    }
    if (workingIssue.description !== currentIssue.description) {
      payload.description = workingIssue.description;
    }
    const statusChanged =
      normalizeStatusValue(workingIssue.status) !==
      normalizeStatusValue(currentIssue.status);
    if (statusChanged) {
      payload.status = normalizeStatusValue(workingIssue.status || currentIssue.status);
    }

    if (Object.keys(payload).length === 0) {
      return true;
    }

    try {
      setStatus(modal, "Saving changes...");
      await patchIssueFields(workingIssue.rawId, payload);
      currentIssue = { ...workingIssue };
      if (onIssueUpdated) {
        onIssueUpdated(workingIssue);
      }
      if (statusChanged) {
        reloadAfterStatusChange();
      }
      return true;
    } catch (err) {
      setStatus(modal, err.message || "Failed to save changes.", true);
      return false;
    }
  };

  const close = async (discard = false) => {
    const modal = backdrop.querySelector(".modal");
    if (modal && !discard) {
      const ok = await persistChanges(modal);
      if (!ok) {
        return;
      }
    }
    backdrop.classList.remove("open");
    backdrop.innerHTML = "";
    currentIssue = null;
    workingIssue = null;
  };

  const openDetail = (issue) => {
    backdrop.innerHTML = "";
    backdrop.appendChild(buildDetail(issue));
    backdrop.classList.add("open");
  };

  const openCreate = () => {
    openDetail(emptyIssue);
  };

  const openEdit = (issue) => {
    openDetail(issue);
  };

  backdrop.addEventListener("click", (evt) => {
    if (evt.target === backdrop) {
      void close();
    }
  });

  return { openDetail, openCreate, openEdit, close, open: openDetail };
};
