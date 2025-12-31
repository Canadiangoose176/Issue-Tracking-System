const issues = [
  {
    title: "Issue Title",
    id: "#1024",
    rawId: 1024,
    database: "Database name",
    createdAt: "Creation Date",
    author: "Author",
    // Default project status
    status: "To Be Done",
    milestone: "Milestone",
    description: "System keeps crashing blah blah blah....",
    comments: [
      { author: "Coder96", date: "12/1/2025", text: "Working on this now" }
    ],
    tags: [
      { label: "Tags", color: "#c256b9" },
      { label: "Tags", color: "#49a3d8" }
    ]
  },
  {
    title: "Fix login redirect loop",
    id: "#2048",
    rawId: 2048,
    database: "Auth DB",
    createdAt: "2025-03-01",
    author: "A. Nash",
    status: "In Progress",
    milestone: "Q1 Hardening",
    description: "Redirect loop observed when session expires mid-login.",
    comments: [
      {
        author: "A. Nash",
        date: "2025-03-02",
        text: "Root cause: stale cookie. Need to clear on 401."
      },
      {
        author: "QA Team",
        date: "2025-03-03",
        text: "Repro on Firefox 123 as well."
      }
    ],
    tags: [
      { label: "Bug", color: "#f52781" },
      { label: "Auth", color: "#49a3d8" }
    ]
  }
];

let issueCounter = 3000;

export const getIssues = () => issues.slice();

export const addMockIssue = () => {
  const idNum = ++issueCounter;
  const newIssue = {
    title: "New mock issue",
    id: `#${idNum}`,
    rawId: idNum,
    database: "New DB",
    createdAt: "Today",
    author: "You",
    status: "To Be Done",
    milestone: "Backlog",
    description: "This is a newly added mock issue.",
    comments: [
      {
        author: "You",
        date: "Today",
        text: "Drafting details for this mock issue."
      }
    ],
    tags: [
      { label: "Mock", color: "#6cee94" },
      { label: "UI", color: "#0f8ccf" }
    ]
  };
  issues.push(newIssue);
  return newIssue;
};
