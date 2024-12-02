package com.example.demo.controller;



import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.RequestMapping;



@Controller
public class HomeController {

    @RequestMapping("/index")
    public String login(){
            return "index";
        }


}
