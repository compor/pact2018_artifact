; ModuleID = 'balanced.cpp'
source_filename = "balanced.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @_Z6serialiPi(i32 %N, i32* %A) #0 {
entry:
  %N.addr = alloca i32, align 4
  %A.addr = alloca i32*, align 8
  %ofs = alloca i32, align 4
  %bal = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 %N, i32* %N.addr, align 4
  store i32* %A, i32** %A.addr, align 8
  store i32 0, i32* %ofs, align 4
  store i32 1, i32* %bal, align 4
  store i32 0, i32* %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %0 = load i32, i32* %i, align 4
  %1 = load i32, i32* %N.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %2 = load i32*, i32** %A.addr, align 8
  %3 = load i32, i32* %i, align 4
  %idxprom = sext i32 %3 to i64
  %arrayidx = getelementptr inbounds i32, i32* %2, i64 %idxprom
  %4 = load i32, i32* %arrayidx, align 4
  %cmp1 = icmp eq i32 %4, 0
  br i1 %cmp1, label %if.then, label %if.else

if.then:                                          ; preds = %for.body
  %5 = load i32, i32* %ofs, align 4
  %add = add nsw i32 %5, 1
  store i32 %add, i32* %ofs, align 4
  br label %if.end

if.else:                                          ; preds = %for.body
  %6 = load i32, i32* %ofs, align 4
  %sub = sub nsw i32 %6, 1
  store i32 %sub, i32* %ofs, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %7 = load i32, i32* %ofs, align 4
  %cmp2 = icmp slt i32 %7, 0
  br i1 %cmp2, label %if.then3, label %if.end4

if.then3:                                         ; preds = %if.end
  store i32 0, i32* %bal, align 4
  br label %if.end4

if.end4:                                          ; preds = %if.then3, %if.end
  br label %for.inc

for.inc:                                          ; preds = %if.end4
  %8 = load i32, i32* %i, align 4
  %inc = add nsw i32 %8, 1
  store i32 %inc, i32* %i, align 4
  br label %for.cond

for.end:                                          ; preds = %for.cond
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0 (trunk 321123)"}
