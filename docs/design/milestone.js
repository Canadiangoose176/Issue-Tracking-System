import { milestoneCardTemplate, actionButtonTemplate } from "./templates.js";
import { renderIssues } from "./issue.js";

export const renderMilestones = ({
  container,
  milestones,
  onOpen,
  onEdit,
  onDelete,
  onAddIssue,
  onToggle,
  onRemoveIssue,
  issueOptions = [],
  issuesMap = new Map(),
  expandedIds = new Set()
}) => {
  if (!container) return;

  const fragment = document.createDocumentFragment();

  milestones.forEach((milestone) => {
    const card = milestoneCardTemplate();
    card.dataset.id = milestone.id;

    const titleEl = card.querySelector('[data-role="title"]');
    titleEl.textContent =
      milestone.id !== undefined ? `${milestone.name} (#${milestone.id})` : milestone.name;
    titleEl.addEventListener("click", (evt) => {
      evt.stopPropagation();
      onOpen && onOpen(milestone);
    });

    card.querySelector('[data-field="description"]').textContent =
      milestone.description || "No description";
    card.querySelector('[data-field="startDate"]').textContent =
      milestone.startDate || "Start ?";
    card.querySelector('[data-field="endDate"]').textContent =
      milestone.endDate || "End ?";

    const issueCount = Array.isArray(milestone.issueIds)
      ? milestone.issueIds.length
      : Array.isArray(milestone.issues)
        ? milestone.issues.length
        : 0;
    card.querySelector('[data-field="issuesCount"]').textContent = `Issues: ${issueCount}`;

    const actionsWrap = card.querySelector('[data-role="actions"]');

    const issueInput = card.querySelector('[data-role="issue-search"]');
    const issueAddBtn = card.querySelector('[data-role="issue-add"]');
    const issueOptionsList = card.querySelector('[data-role="issue-options"]');
    const issuesContainer = card.querySelector('[data-role="issues"]');
    const toggleBtn = card.querySelector('[data-role="toggle"]');
    const body = card.querySelector('[data-role="body"]');

    const datalistId = `issue-options-${milestone.id || "new"}`;
    issueOptionsList.id = datalistId;
    issueInput.setAttribute("list", datalistId);

    issueOptionsList.replaceChildren(
      ...issueOptions.map((issue) => {
        const opt = document.createElement("option");
        const raw = issue.rawId !== undefined ? issue.rawId : issue.id;
        const cleanId = `${raw}`.replace(/^#/, "");
        opt.value = cleanId;
        opt.label = `#${cleanId} — ${issue.title || issue.name || ""}`;
        return opt;
      })
    );

    const tryAddIssue = async () => {
      const value = issueInput.value.trim();
      if (!value) return;
      const success = await onAddIssue?.(milestone, value);
      if (success) {
        issueInput.value = "";
      }
    };

    issueAddBtn?.addEventListener("click", async (evt) => {
      evt.stopPropagation();
      await tryAddIssue();
    });

    issueInput.addEventListener("keydown", (evt) => {
      if (evt.key === "Enter") {
        evt.preventDefault();
        issueAddBtn?.click();
      }
    });

    issueInput.addEventListener("blur", () => {
      setTimeout(() => {
        tryAddIssue();
      }, 50);
    });

    const editBtn = actionButtonTemplate("Edit", "edit");
    editBtn.addEventListener("click", (evt) => {
      evt.stopPropagation();
      onEdit && onEdit(milestone);
    });
    actionsWrap.appendChild(editBtn);

    const deleteBtn = actionButtonTemplate("Delete", "delete");
    deleteBtn.addEventListener("click", (evt) => {
      evt.stopPropagation();
      onDelete && onDelete(milestone);
    });
    actionsWrap.appendChild(deleteBtn);

    const setExpanded = (expanded) => {
      card.dataset.expanded = expanded ? "true" : "false";
      body.classList.toggle("hidden", !expanded);
      toggleBtn.textContent = expanded ? "▾" : "▸";
    };

    const renderIssueList = () => {
      const issueList = issuesMap.get(milestone.id) || [];
      if (!issueList.length) {
        issuesContainer.textContent = "No issues linked yet.";
        return;
      }
      renderIssues({
        container: issuesContainer,
        issues: issueList,
        onOpen: () => {},
        onEdit: () => {},
        onDelete: () => {},
        actionBuilder: (wrap, issue) => {
          wrap.innerHTML = "";
          const removeBtn = actionButtonTemplate("Remove", "delete");
          removeBtn.addEventListener("click", (evt) => {
            evt.stopPropagation();
            onRemoveIssue && onRemoveIssue(milestone, issue);
          });
          wrap.appendChild(removeBtn);
        }
      });
    };

    toggleBtn.addEventListener("click", async (evt) => {
      evt.stopPropagation();
      const next = card.dataset.expanded !== "true";
      setExpanded(next);
      await onToggle?.(milestone, next);
      if (next) {
        renderIssueList();
      }
    });

    card.addEventListener("dblclick", (evt) => {
      evt.stopPropagation();
      onEdit && onEdit(milestone);
    });

    card.addEventListener("click", async (evt) => {
      const withinHeader = evt.target.closest(".milestone-header");
      if (withinHeader && !evt.target.closest("button")) {
        const next = card.dataset.expanded !== "true";
        setExpanded(next);
        await onToggle?.(milestone, next);
        if (next) {
          renderIssueList();
        }
      }
    });

    const startExpanded = expandedIds.has(milestone.id);
    setExpanded(startExpanded);
    if (startExpanded) {
      renderIssueList();
    }

    fragment.appendChild(card);
  });

  if (typeof container.replaceChildren === "function") {
    container.replaceChildren(fragment);
  } else {
    container.innerHTML = "";
    container.appendChild(fragment);
  }
};
