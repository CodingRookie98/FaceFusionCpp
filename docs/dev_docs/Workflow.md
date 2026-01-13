### 工作流程

#### 流程概述
本工作流程支持两种路径：**评估路径**（需要代码质量评估）和**直接实现路径**（无需评估）。流程设计遵循"批量确认、并行处理、快速通道"原则，并支持根据用户意愿灵活调整确认机制。

#### 评估路径（需要代码质量评估）

**阶段一：代码评估**
1. 评估代码质量
2. 编写评估文档（路径：`/docs/dev_docs/evaluation/C++_evaluation_{title}.md`）
3. 等待用户确认评估结果（若用户要求无需确认则跳过）
4. 若用户确认评估结果，进入阶段二；否则，根据反馈重新评估（返回步骤1）

**阶段二：计划制定**
5. 根据评估结果制定实施计划
6. 用户确认计划（若用户要求无需确认则跳过）
   - 若确认，进入阶段三
   - 若拒绝，重新制定计划（返回步骤5）

**阶段三：任务文档生成**
7. 根据计划批量生成子任务文档
   - 路径：`/docs/dev_docs/plan/{plan_name}/task/C++_task_{task_name}.md`
   - 格式：中文 Markdown
   - 支持批量生成，减少交互次数
8. 用户确认子任务文档（可批量确认或逐个确认；若用户要求无需确认则跳过）
   - 若确认，进入阶段四
   - 若拒绝，修改任务文档（返回步骤7）

**阶段四：任务实现**
9. 批量实现子任务（根据用户确认的任务文档）
10. 用户确认子任务实现（可批量确认或逐个确认；若用户要求无需确认则跳过）
    - 若确认，更新任务状态，继续实现剩余子任务
    - 若拒绝，重新实现子任务（返回步骤9）
11. 提交子任务更改到git（仅提交代码文件，不提交文档文件）
12. 更新任务文档状态
    - 将对应的子任务文档状态标记为"已完成"
    - 记录完成时间与相关 commit ID

**阶段五：完成验收**
13. 所有子任务实现完成后，用户确认任务完成（若用户要求无需确认则跳过）
14. 更新计划状态为"已完成"
15. 流程结束

#### 直接实现路径（无需代码评估）

**阶段一：计划制定**
1. 直接制定实施计划
2. 用户确认计划（若用户要求无需确认则跳过）
   - 若确认，进入阶段二
   - 若拒绝，重新制定计划（返回步骤1）

**阶段二：任务文档生成**
3. 根据计划批量生成子任务文档
   - 路径：`/docs/dev_docs/plan/{plan_name}/task/C++_task_{task_name}.md`
   - 格式：中文 Markdown
4. 用户确认子任务文档（可批量确认或逐个确认；若用户要求无需确认则跳过）
   - 若确认，进入阶段三
   - 若拒绝，修改任务文档（返回步骤3）

**阶段三：任务实现**
5. 批量实现子任务（根据用户确认的任务文档）
6. 用户确认子任务实现（可批量确认或逐个确认；若用户要求无需确认则跳过）
    - 若确认，更新任务状态，继续实现剩余子任务
    - 若拒绝，重新实现子任务（返回步骤5）
7. 提交子任务更改到git（仅提交代码文件，不提交文档文件）
8. 更新任务文档状态
   - 将对应的子任务文档状态标记为"已完成"
   - 记录完成时间与相关 commit ID

**阶段四：完成验收**
9. 所有子任务实现完成后，用户确认任务完成（若用户要求无需确认则跳过）
10. 更新计划状态为"已完成"
11. 流程结束

#### 流程特性说明

**1. 双路径设计**
- 评估路径：适用于需要代码质量评估的场景，确保代码质量符合要求
- 直接实现路径：适用于简单任务或用户明确无需评估的场景，提高效率

**2. 批量处理机制**
- 子任务文档支持批量生成和确认，减少交互次数
- 用户可选择批量确认或逐个确认，根据任务复杂度灵活选择

**3. 快速通道**
- 对于简单任务（如单个子任务），可跳过部分确认环节，直接进入实现阶段
- 支持用户自定义确认粒度

**4. 状态管理**
- 每个阶段完成后更新状态，确保流程可追溯
- 支持回滚到任意阶段，便于调整和修正

**5. 异常处理**
- 任何阶段被拒绝时，都有明确的回退路径
- 支持部分完成后的继续执行，避免全量重做

**6. 版本控制**
- 每个子任务完成后，自动提交代码更改到git
- 仅提交代码文件，不提交文档和中间文件
- 确保代码版本的可追溯性和安全性

**7. 灵活确认机制**
- **原则**：流程中的确认环节执行取决于用户的显式要求。
- **显式确认**：若用户显式要求确认，则所有相关的确认节点（如评估、计划、文档、实现结果）必须等待用户确认。
- **无需确认**：若用户显式要求无需确认，则跳过确认节点，直接进入下一阶段。

#### 流程图

```mermaid
graph TD
    Start[工作流程入口] --> IsEvaluationNeeded{是否需要代码评估?}
    
    IsEvaluationNeeded -->|是| EvaluationPath[评估路径]
    IsEvaluationNeeded -->|否| DirectPath[直接实现路径]
    
    %% 评估路径
    EvaluationPath --> EvalCode[评估代码质量]
    EvalCode --> WriteEvalDoc[编写评估文档]
    WriteEvalDoc --> CheckConfirm1{用户要求确认?}
    
    CheckConfirm1 -->|是| UserConfirmEval{用户确认评估?}
    CheckConfirm1 -->|否| CreatePlan
    
    UserConfirmEval -->|确认| CreatePlan[制定实施计划]
    UserConfirmEval -->|拒绝| ReEval[重新评估或返回评估]
    ReEval --> EvalCode
    
    %% 直接实现路径
    DirectPath --> CreatePlan
    
    %% 计划阶段
    CreatePlan --> CheckConfirm2{用户要求确认?}
    CheckConfirm2 -->|是| UserConfirmPlan{用户确认计划?}
    CheckConfirm2 -->|否| GenTaskDocs
    
    UserConfirmPlan -->|确认| GenTaskDocs[生成子任务文档]
    UserConfirmPlan -->|拒绝| ReCreatePlan[重新制定计划]
    ReCreatePlan --> CreatePlan
    
    %% 任务生成阶段
    GenTaskDocs --> CheckConfirm3{用户要求确认?}
    CheckConfirm3 -->|是| UserConfirmTaskDocs{用户确认子任务文档?}
    CheckConfirm3 -->|否| ImplementTasks
    
    UserConfirmTaskDocs -->|确认| ImplementTasks[批量实现子任务]
    UserConfirmTaskDocs -->|拒绝| ModifyTaskDocs[修改任务文档]
    ModifyTaskDocs --> GenTaskDocs
    
    %% 任务实现阶段
    ImplementTasks --> CheckConfirm4{用户要求确认?}
    CheckConfirm4 -->|是| UserConfirmImpl{用户确认子任务实现?}
    CheckConfirm4 -->|否| CommitGit
    
    UserConfirmImpl -->|确认| CommitGit[提交更改到git]
    UserConfirmImpl -->|拒绝| ReImplement[重新实现子任务]
    ReImplement --> ImplementTasks
    
    CommitGit --> UpdateTaskDoc[更新任务文档状态]
    UpdateTaskDoc --> UpdateStatus[更新流程状态]
    
    UpdateStatus --> AllTasksDone{所有子任务完成?}
    
    AllTasksDone -->|否| NextTask[继续下一子任务]
    NextTask --> ImplementTasks
    
    AllTasksDone -->|是| CheckConfirm5{用户要求确认?}
    CheckConfirm5 -->|是| UserConfirmDone[用户确认完成]
    CheckConfirm5 -->|否| UpdatePlanStatus
    
    %% 完成阶段
    UserConfirmDone --> UpdatePlanStatus[更新计划状态]
    UpdatePlanStatus --> End[流程结束]
```
