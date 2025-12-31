const htmlToElement = (html) => {
  const tpl = document.createElement("template");
  tpl.innerHTML = html.trim();
  return tpl.content.firstElementChild;
};

export const issueCardTemplate = () =>
  htmlToElement(`
    <article class="issue-card">
      <h2 class="issue-title">
        <button class="issue-title-btn" data-role="title"></button>
      </h2>
      <div class="issue-meta">
        <span data-field="database"></span>
        <span data-field="createdAt"></span>
        <span data-field="author"></span>
        <span data-field="milestone"></span>
      </div>
      <div class="card-actions">
        <div class="tags" data-role="tags"></div>
        <div class="actions-right" data-role="actions"></div>
      </div>
    </article>
  `);

export const tagTemplate = (tag, extraClasses = []) => {
  const el = htmlToElement(`<span class="tag"></span>`);
  const classes = Array.isArray(extraClasses) ? extraClasses : [extraClasses];
  classes.filter(Boolean).forEach((cls) => el.classList.add(cls));
  el.textContent = (tag && (tag.label || tag.tag)) || "Tag";
  el.style.backgroundColor = (tag && tag.color) || "#49a3d8";
  return el;
};

export const tagChipTemplate = (tag, extraClasses = []) => {
  const classes = Array.isArray(extraClasses) ? extraClasses : [extraClasses];
  return tagTemplate(tag, ["chip", ...classes]);
};

export const actionButtonTemplate = (label, variant) => {
  const el = htmlToElement(`<button type="button" class="action-btn"></button>`);
  el.textContent = label;
  if (variant) {
    el.classList.add(variant);
  }
  return el;
};

export const modalBackdropTemplate = () =>
  htmlToElement(`<div class="modal-backdrop"></div>`);

export const modalTemplate = () =>
  htmlToElement(`
    <div class="modal">
      <div class="modal-header">
        <span data-field="heading"></span>
        <button type="button" class="modal-close" data-role="cancel-modal">Cancel</button>
      </div>
      <div class="detail-grid">
        <div class="detail-main">
          <div class="detail-view" data-role="detail-view">
            <div class="detail-title-box" data-field="titleBox"></div>
            <div class="detail-description" data-field="description"></div>
          </div>
          <div class="author-row" data-role="author-select"></div>
          <div class="form-status" data-role="form-status" aria-live="polite"></div>
        </div>
        <div class="detail-sidebar" data-role="sidebar"></div>
      </div>
      <div data-role="tag-manager-region" class="hidden"></div>
      <button type="button" class="add-comment">Add Comment</button>
      <div data-role="comment-form-region"></div>
      <div class="comments" data-role="comments"></div>
    </div>
  `);

export const pillTemplate = (label, className) => {
  const el = htmlToElement(`<button type="button" class="detail-pill"></button>`);
  el.textContent = label;
  if (className) {
    el.classList.add(className);
  }
  return el;
};

export const commentCardTemplate = (comment) => {
  const el = htmlToElement(`
    <div class="comment-card">
      <div class="comment-header">
        <span class="comment-meta"></span>
        <button type="button" class="comment-delete">Delete</button>
      </div>
      <div class="comment-body"></div>
    </div>
  `);
  if (comment.id !== undefined && comment.id !== null) {
    el.dataset.commentId = comment.id;
  }
  el.querySelector(".comment-meta").textContent =
    `${comment.author} · ${comment.date}`;
  el.querySelector(".comment-body").textContent = comment.text;
  return el;
};

export const commentFormTemplate = () =>
  htmlToElement(`
    <form class="comment-form" data-role="comment-form">
      <label>
        <span>Author</span>
        <select data-field="comment-author"></select>
      </label>
      <label>
        <span>Comment</span>
        <textarea data-field="comment-text" rows="3" placeholder="Add a comment"></textarea>
      </label>
      <div class="comment-actions">
        <button type="submit" class="comment-submit">Post Comment</button>
        <button type="button" class="comment-cancel" data-role="cancel-comment">Cancel</button>
      </div>
    </form>
  `);

export const tagsPillTemplate = (tags = []) => {
  const el = htmlToElement(`
    <div class="detail-pill tags" data-role="tags-pill" role="button" tabindex="0">
      <div class="tags-pill__label">Tags</div>
      <div class="tags-pill__chips" data-role="tags-list"></div>
    </div>
  `);
  const wrap = el.querySelector('[data-role="tags-list"]');
  const values = Array.isArray(tags) ? tags : [];
  if (!values.length) {
    wrap.appendChild(
      htmlToElement('<div class="tags-pill__empty">No tags yet</div>')
    );
  } else {
    values.forEach((tag) => wrap.appendChild(tagChipTemplate(tag)));
  }
  return el;
};

export const milestoneCardTemplate = () =>
  htmlToElement(`
    <article class="row milestone-row" data-expanded="false">
      <div class="milestone-header">
        <button type="button" class="toggle" data-role="toggle" aria-label="Toggle milestone">▸</button>
        <div class="meta">
          <strong data-role="title"></strong>
          <span data-field="startDate"></span>
          <span>→</span>
          <span data-field="endDate"></span>
        </div>
        <div class="meta secondary">
          <span data-field="description"></span>
          <span data-field="issuesCount"></span>
        </div>
        <div class="actions" data-role="actions"></div>
      </div>
      <div class="milestone-body hidden" data-role="body">
        <div class="add-issue-row">
          <label class="visually-hidden" data-role="issue-label">Issue</label>
          <input type="text" class="input-issue" data-role="issue-search" placeholder="Search or enter issue id">
          <datalist data-role="issue-options"></datalist>
          <button type="button" class="btn add" data-role="issue-add">Add Issue</button>
        </div>
        <div class="issues" data-role="issues"></div>
      </div>
    </article>
  `);

export const tagManagerTemplate = () =>
  htmlToElement(`
    <div class="tag-manager" data-role="tag-manager">
      <div class="tag-manager__search-row">
        <input
          type="search"
          class="tag-manager__search"
          placeholder="Search tags"
          data-role="tag-search"
        />
        <button type="button" class="tag-manager__close" data-role="tag-manager-close">X</button>
      </div>
      <div class="tag-manager__list" data-role="tag-list"></div>
      <button type="button" class="tag-manager__create-trigger" data-role="open-tag-create">
        Create project tag
      </button>
      <div class="tag-manager__create-host" data-role="tag-create-host"></div>
    </div>
  `);

export const tagManagerRowTemplate = (tag) => {
  const el = htmlToElement(`
    <div class="tag-manager__row">
      <span class="tag-manager__chip"></span>
      <span class="tag-manager__name"></span>
      <button type="button" class="tag-manager__delete">Delete</button>
    </div>
  `);
  const chip = el.querySelector(".tag-manager__chip");
  const name = el.querySelector(".tag-manager__name");
  const normalized = tag || {};
  chip.textContent = normalized.label || normalized.tag || "Tag";
  chip.style.backgroundColor = normalized.color || "#49a3d8";
  name.textContent = normalized.label || normalized.tag || "Tag";
  return el;
};

export const tagEditorFormTemplate = () =>
  htmlToElement(`
    <form class="tag-manager__create tag-manager__edit-form" data-role="tag-create-form">
      <label class="tag-manager__create-label">
        <span>Tag name</span>
        <input type="text" placeholder="bug, backend, docs" data-field="tag-name" />
      </label>
      <div class="tag-manager__color-row">
        <label>Color</label>
        <input type="color" value="#49a3d8" data-field="tag-color" />
      </div>
      <div class="tag-manager__actions">
        <button type="submit" class="tag-manager__save">Save tag</button>
        <button type="button" class="tag-manager__cancel" data-role="cancel-tag-create">Cancel</button>
      </div>
    </form>
  `);
