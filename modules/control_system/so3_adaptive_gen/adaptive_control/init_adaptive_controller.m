function ctrl = init_adaptive_controller()
filter_obj = make_iter_trans(2);
ctrl = struct('t',0,'x',[0; 0; 1.0; 0.0; 0.0; 0 ; 0],'Gamma',0,...
'P',[0 0; 0 0],'Am',[0 0;0 0],'u',0,'b',[0;0],'kg',0,'kg_rate',0,...
'u_filter',filter_obj,'g_filter',filter_obj,'inited',false,'err',[0;0],...
'eta',0,'r',0,'rdot',0,'g',[0;0],'g_by_x',[0 0;0 0],'km',[0;0],...
'x_real',[0;0],'u_lead',make_lead_obj(1,4),'u_lead_enable',true);
%coder.cstructname(ctrl, 'AdaptiveCtrlT');
end