# CLAUDE.md — repo conventions for cutils

## Git commits

Use [Conventional Commits](https://www.conventionalcommits.org/). Every commit
must reference a GitHub issue/ticket number in the scope, in the form
`<type>(#<issue>): <description>`.

- The issue number is mandatory. If a change has no issue yet, create one first
  (via `gh issue create`) and reference it.
- Multiple commits may reference the same issue when they're all part of that
  ticket's work (e.g. a fix followed by a related test tweak).
- Keep the subject line ≤ 80 characters. Use the imperative mood
  ("add", "fix", "replace"), no trailing period.
- Body (optional): wrap at 72–80 chars, explain *what* and *why* (not *how*).
  Include a `Refs: #<issue>` trailer when the subject scope already names the
  issue and you want to cross-reference additional issues.

### Types

`feat` `fix` `chore` `docs` `style` `refactor` `perf` `test` `build` `ci`

### Examples

```
fix(#2): gate USE_STD_PRINTF compile definition behind the option
chore(#4): replace googletest submodule with FetchContent
docs(#1): document state_event_loop event processing flow
```

### Workflow

1. Create (or pick) the issue for the work.
2. Branch from `master` (e.g. `switch-googletest-to-fetchcontent`).
3. Commit with `<type>(#<issue>): <desc>`.
4. Verify build + tests before pushing.
5. Open a PR referencing the issue.
